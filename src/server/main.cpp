#include <chatserver.hpp>
#include <chatservice.hpp>
#include <public.hpp>


int main(){
    InetAddress addr("127.0.0.1", 6000);
    EventLoop loop;
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();
    return 0;
}