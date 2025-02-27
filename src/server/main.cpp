#include <chatserver.hpp>
#include <chatservice.hpp>
#include <public.hpp>
#include <signal.h>

// 处理 ctl + c 信号， 重置user的状态信息
void resetHandler(int)
{
    ChatService::getInstance()->reset();
    exit(0);
}


int main(){
    // 注册 ctl + c 信号
    signal(SIGINT, resetHandler);

    InetAddress addr("127.0.0.1", 6000);
    EventLoop loop;
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();
    return 0;
}