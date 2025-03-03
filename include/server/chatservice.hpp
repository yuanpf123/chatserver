#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>


using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "usermodel.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
using json = nlohmann::json;

using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

class ChatService
{
public:
    // 获取单例对象的调用接口
    static ChatService *getInstance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //一对一聊天
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常，业务重置方法
    void reset();

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群聊
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // 获取消息id对应的handler
    MsgHandler getHandler(int msgid);

private:
    ChatService();
    // 存储消息id及其对应的回调函数handler
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作类对象
    UserModel _userModel;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;

};

#endif