#ifndef CHATSERVICE_CPP

#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
using namespace muduo;
using namespace std;

// 初始化，将消息id与对应的回调函数进行绑定
ChatService::ChatService()
{
    // 登录业务触发
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // 注册业务触发
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
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
    user.setState("offline");
    _userModel.updateState(user);
}


#endif