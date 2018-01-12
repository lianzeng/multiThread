
#include "TcpConnection.hpp"
#include "Channel.hpp"
#include "SocketsOps.hpp"
#include "../utils/Logging.hpp"

namespace net
{

TcpConnection::TcpConnection(EventLoop *loop, int sockfd):
  loop_(loop),
  channelPtr(new Channel(loop, sockfd)),
  socketPtr(new Socket(sockfd)),
  sendBuffer(sockfd),
  receivedBuffer(sockfd),
  states_(Connecting)

{
  using namespace std::placeholders;
  channelPtr->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
  channelPtr->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channelPtr->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channelPtr->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
}

void TcpConnection::send(std::string&& msg)
{
    loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, std::move(msg)));
}

void TcpConnection::sendInLoop(const std::string & msg)
{
    loop_->assertInLoopThread();

    SendBuffer::Result sendStatues = {SendBuffer::Success, 0};//<ok, wroteBytes>
    if(sendingQueueEmpty())
    {
        sendStatues = sendBuffer.send( msg.data(), msg.size());
    }

    auto wroteBytes = sendStatues.second;
    size_t remainData = msg.size() - wroteBytes;

    if(remainData == 0 && sendCompleteCallback_)
    {
        loop_->queueInLoop(std::bind(sendCompleteCallback_, shared_from_this()));
    }
    else if(remainData > 0 && sendStatues.first)
    {
        auto trigHighWaterMark = sendBuffer.appendata(&msg[wroteBytes], remainData);
        if(trigHighWaterMark && highWaterMarkCallback_)
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), sendBuffer.readbleBytes()));

        if(!channelPtr->isWriting())
            channelPtr->enableWriting();
    }
}


void TcpConnection::connectEstablished() {
    LOG_INFO <<"TcpConnection::connectEstablished: fd = " << channelPtr->fd();
    loop_->assertInLoopThread();
    setState(Connected);
    channelPtr->enableReading();
    connectionCallback_(shared_from_this());

}

void TcpConnection::handleRead(TimeStamp receiveTime) {
    LOG_INFO <<"TcpConnection::handleRead: fd = " << channelPtr->fd();
    loop_->assertInLoopThread();
    auto readBytes = receivedBuffer.receive();
    if(readBytes > 0)
        messageCallback_(shared_from_this(), &receivedBuffer, receiveTime);
    else if(readBytes == 0) //peer close
        handleClose();
    else
        handleError();
}

void TcpConnection::handleWrite() {

    LOG_INFO <<"TcpConnection::handleWrite: fd = " << channelPtr->fd();
    loop_->assertInLoopThread();
    auto result = sendBuffer.send();
    if(result.first && sendBuffer.readbleBytes() == 0)
    {
        channelPtr->disableWriting();
        if(sendCompleteCallback_)
            loop_->queueInLoop(std::bind(sendCompleteCallback_, shared_from_this()));
        if(states_ == Disconnecting)
            shutDownInLoop();
    }
}

void TcpConnection::handleClose() {

    LOG_INFO <<"TcpConnection::handleClose: fd = " << channelPtr->fd();
    loop_->assertInLoopThread();
    setState(Disconnected);
    channelPtr->disableEvents();
    closeCallback_(shared_from_this());
}

void TcpConnection::connectionDestroyed()
{
    LOG_INFO <<"TcpConnection::connectionDestroyed: fd = " << channelPtr->fd();
    loop_->assertInLoopThread();
    channelPtr->removeSelf();
}

void TcpConnection::handleError() {
    loop_->assertInLoopThread();
    int err = sockets::getSocketError(channelPtr->fd());
    LOG_ERROR << "TcpConnection::handleError " << " - SO_ERROR = " << err << " " << logg::strerror_tl(err);
}


bool TcpConnection::sendingQueueEmpty() const {
        return !channelPtr->isWriting() && sendBuffer.readbleBytes() == 0;
}

void TcpConnection::shutDownInLoop() {
    LOG_INFO <<"TcpConnection::shutDownInLoop: fd = " << channelPtr->fd();
    if(!channelPtr->isWriting())
        socketPtr->shutdownWrite();

}


}
