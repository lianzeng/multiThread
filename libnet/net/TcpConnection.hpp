
#ifndef _TCP_CONNECTION_HPP
#define _TCP_CONNECTION_HPP

#include <memory>
#include <string>
#include "EventLoop.hpp"
#include "../utils/TimeStamp.hpp"
#include "Socket.hpp"


namespace net
{

class Channel;

//TCP connection: for both client and server usage, expose to user;
class TcpConnection
{
public:
using ChannelPtr = std::unique_ptr<Channel>;


TcpConnection(EventLoop* loop, int sockfd);
~TcpConnection();

void send(std::string&& msg);
void sendInLoop(std::string&& msg);

void handleRead(TimeStamp);
void handleWrite();
void handleClose();
void handleError();

void connectEstablished();//called when 1)tcp server accept new connection or 2)tcp client connect sucessfully.

private:
TcpConnection(const TcpConnection&) = delete;
TcpConnection& operator=(const TcpConnection&) = delete;


EventLoop* loop_;
ChannelPtr channelPtr;
std::unique_ptr<Socket> socketPtr;
    //sendBuffer
    //receivedBuffer
};


using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

}

#endif
