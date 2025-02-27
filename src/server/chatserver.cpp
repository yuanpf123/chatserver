#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
#include <iostream>
#include <muduo/base/Logging.h>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

/*
基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造函数需要什么参数，输出chatserver的构造函数
4.在当前服务器的构造函数中。注册处理连接和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/

ChatServer::ChatServer(EventLoop *loop,               // 事件循环
                       const InetAddress &listenAddr, // IP + Port
                       const string &nameArg)         // 服务器的名字
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 发生链接建立和断开事件时对应的回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnectionCallback, this, _1));
    // 发生消息读写事件时对应的回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessageCallback, this, _1, _2, _3));

    // 设置服务器端线程数量
    _server.setThreadNum(4); // 一个连接。三个worker
}

// 启动函数，开启事件循环
void ChatServer::start()
{
    _server.start();
}

// 处理链接建立和断开时的回调函数
void ChatServer::onConnectionCallback(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
    else
    {
        cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << endl;
    }
}

// 处理读写事件时的回调函数
void ChatServer::onMessageCallback(const TcpConnectionPtr &conn,
                                   Buffer *buffer, // 读写缓冲区
                                   Timestamp time) // 接收到数据的时间
{
    string buf = buffer->retrieveAllAsString();
    // json数据的反序列化
    // json js = json::parse(buf);
    json js;
    try
    {
        js = json::parse(buf);
        LOG_INFO << "Received JSON: " << js.dump();
    }
    catch (const json::parse_error &e)
    {
        LOG_ERROR << "JSON parse error: " << e.what();
        return;
        // 返回错误响应或执行其他处理
    }
    /*
        获取收到的消息id对应的事件处理器
        get<int>()将json数据转换为对应的int类型
        通过js["msgid"] 获取对应的业务handler
    */
    // 处理消息id对应的消息事件
    // 检查是否包含 msgid 字段，并确保其为数字类型
    if (js.contains("msgid") && js["msgid"].is_number_integer())
    {
        // 获取消息处理器
        auto msgHandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
        // 处理消息
        msgHandler(conn, js, time);
    }
    else
    {
        // 如果没有 msgid 或类型不匹配，记录错误并返回错误响应给客户端
        LOG_ERROR << "Invalid or missing msgid in received JSON!";

        json errorResponse = {
            {"error", "Invalid msgid"},
            {"message", "msgid is missing or not an integer"}};
        conn->send(errorResponse.dump()); // 发送错误响应给客户端
    }
}