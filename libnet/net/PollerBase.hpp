
#ifndef _POLLER_BASE_HPP
#define  _POLLER_BASE_HPP

#include <map>
#include <vector>
#include "EventLoop.hpp"
#include "../utils/TimeStamp.hpp"


namespace net
{

class Channel;

class PollerBase
{
public:
using ChannelList = std::vector<Channel*> ;

PollerBase(EventLoop* loop);
virtual ~PollerBase();

   static PollerBase* newDefaultPoller(EventLoop* loop);
   

  virtual  ChannelList poll(const int timeoutMs, TimeStamp& retTime) = 0;

  virtual void updateChannel(Channel* channel) = 0;

  virtual void removeChannel(Channel* channel) = 0;

  void assertInLoopThread() const
  {
      ownerLoop_->assertInOwnerThread();
  }

private:
    virtual void fillActiveChannels(int numEvents , ChannelList& activeChannels) const = 0;

PollerBase(const PollerBase&) = delete;
PollerBase& operator=(const PollerBase&) = delete;



protected:

using ChannelMap =  std::map<int, Channel*> ;//fd->channel*

ChannelMap channelMap_;


private:
EventLoop* ownerLoop_;

};

}

#endif

