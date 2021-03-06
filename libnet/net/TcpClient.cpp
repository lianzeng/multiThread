#include "TcpClient.hpp"
#include "Connector.hpp"
#include "../utils/Logging.hpp"
#include "TcpConnection.hpp"
#include <functional>


namespace net
{

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr):
  loop_(loop),
  connector_(new Connector(loop, serverAddr))
{
  using namespace std::placeholders;
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
}

TcpClient::~TcpClient()
{
}

void TcpClient::connect()
{
  LOG_INFO <<" TcpClient start connect ";
  connector_->start();
}

void TcpClient::newConnection(int sockfd)
{
    LOG_INFO <<"TcpClient connect to server succesfully.";
    using namespace std::placeholders;
    loop_->assertInOwnerThread();
    TcpConnectionPtr connectionPtr(new TcpConnection(loop_, sockfd));
    connectionPtr->setMessageCallback(messageCallback_);
    connectionPtr->setSendCompleteCallback(sendCompleteCallback_);
    connectionPtr->setConnectionCallback(connectionCallback_);
    connectionPtr->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connectionPtr_ = connectionPtr;
    }

    loop_->runInLoop(std::bind(&TcpConnection::connectEstablished, connectionPtr));

}

void TcpClient::removeConnection(const TcpConnectionPtr & conn)
{
    assert(loop_ == conn->ownerLoop());

    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(connectionPtr_ == conn);
        connectionPtr_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));

}


}
