

#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace muduo;
using namespace std;

// 初始化，将消息id与对应的回调函数进行绑定
ChatService::ChatService()
{
    // 登录业务触发
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // 注册业务触发
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    //
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});

    // 添加好友业务
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 把online的用户设置成offline
    _userModel.resetState();
}

// 获取单例对象的调用接口
ChatService *ChatService::getInstance()
{
    static ChatService service;
    return &service;
}

// 获取消息id对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);

    // 记录错误日志，消息id没有对应的事件回调handler
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            // LOG_ERROR<<"msgid:"<<msgid<<"找不到对应的handler!";
            LOG("找不到对应的handler!");
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do login service!!!";
    int id = js["id"].get<int>();
    string passwd = js["password"];

    User user = _userModel.queryById(id);
    if (user.getId() != -1 && user.getPasswd() == passwd)
    {

        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "该用户已经登录，不允许重复登录";
            conn->send(response.dump());
            return;
        }
        else
        {
            // 登录成功，记录用户连接信息 锁的粒度
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            //mysql保证了线程安全
            user.setState("online");
            _userModel.updateState(user);

            // 返回登录信息 线程局部存储
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            response["state"] = user.getState();

            // 查询该用户的好友信息
            vector<User> vec = _friendModel.query(id);
            if (!vec.empty())
            {
                vector<string> vec2;
                for (User &user : vec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            } 
            // 查询该用户的群组信息
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if(!groupVec.empty()){
                vector<string> groupVec2;
                for(Group &group: groupVec){
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getGroupName();
                    js["groupdesc"] = group.getGroupDesc();
                    vector<string> userVec; // 群组用户信息
                    for(GroupUser &user: group.getGroupUsers()){
                        json js;
                        js["uid"] = user.getId();
                        js["uname"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userVec.push_back(js.dump());
                    }
                    js["gusers"] = userVec;
                    groupVec2.push_back(js.dump());

                }
                response["groups"] = groupVec2;
            }

            // 查询该用户是否有离线消息
            vector<string> messages = _offlineMsgModel.query(id);
            if (!messages.empty())
            {
                response["offlinemsg"] = messages;
                // 删除用户的离线消息
                _offlineMsgModel.remove(id);
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败，用户名或密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do reg service!!!";
    string name = js["name"];
    string passwd = js["password"];

    User user;
    user.setName(name);
    user.setPasswd(passwd);

    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        // 构建返回的json对象
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "error";
        conn->send(response.dump());
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();it++){
            if(it->second==conn){
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updateState(user);
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it!=_userConnMap.end()){
            //对方在线，转发消息
            it->second->send(js.dump());
            return;
        }
    }
    
    //toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump()); 
}


//添加好友
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    _friendModel.insert(userid, friendid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 群id此时未知，因此传入-1
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupModel.addGroup(userid, groupid, "normal");
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    string text = js["text"];

    // 查询该群组的用户信息
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    // 锁，保证用户连接map的安全操作，因为可能会多个用户同时发送群消息
    lock_guard<mutex> lock(_connMutex);
    // 转发群消息 给群组中的每个用户
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 存储离线消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}