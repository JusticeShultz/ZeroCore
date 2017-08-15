///////////////////////////////////////////////////////////////////////////////
/// 
/// Authors: Chris Peters
/// Copyright 2010-2011, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"

namespace Zero
{

namespace Z
{
ThreadDispatch* gDispatch = nullptr;
}

//-------------------------------------------------------------------ThreadDispatch
ThreadDispatch::ThreadDispatch()
{
  Z::gDispatch = this;
}

ThreadDispatch::~ThreadDispatch()
{
  ClearEvents();
}

void ThreadDispatch::DispatchOn(HandleParam object, EventDispatcher* eventDispatcher, StringParam eventId, Event* event)
{
  //If this is null, ThreadDispatch has been shutdown. To prevent
  //crashes with destruction order just do nothing.
  if(this == nullptr)
    return;

  QueuedEvent queuedEvent;
  queuedEvent.Object = object;
  queuedEvent.EventToSend = event;
  queuedEvent.EventDispatcherOn = eventDispatcher;
  queuedEvent.EventId  = eventId;

  mLock.Lock();
  mEvents.PushBack(queuedEvent);
  mLock.Unlock();
}

void ThreadDispatch::DispatchEvents()
{
  Array<QueuedEvent> eventsToDispatch;

  //To avoid dead lock pull out all message before dispatching
  //(dispatching may add more events)
  mLock.Lock();
  eventsToDispatch.Swap(mEvents);
  mLock.Unlock();

  forRange(QueuedEvent& queuedEvent, eventsToDispatch.All())
  {
    //Check to see if the object is still alive
    if(queuedEvent.Object.IsNull() == false)
      queuedEvent.EventDispatcherOn->Dispatch(queuedEvent.EventId, queuedEvent.EventToSend);

    //delete the event
    delete queuedEvent.EventToSend;
  }
  eventsToDispatch.Clear();
}

void ThreadDispatch::ClearEvents()
{
  Array<QueuedEvent> eventsToDispatch;

  mLock.Lock();
  eventsToDispatch.Swap(mEvents);
  mLock.Unlock();

  forRange(QueuedEvent& queuedEvent, eventsToDispatch.All())
  {
    delete queuedEvent.EventToSend;
  }
  eventsToDispatch.Clear();
}


void StartThreadSystem()
{
  Z::gDispatch = new ThreadDispatch();
  Z::gJobs = new JobSystem();
}

void ShutdownThreadSystem()
{
  // This is important that the jobs are deleted first, because the job threads could be using the gDispatch
  SafeDelete(Z::gJobs);
  SafeDelete(Z::gDispatch);
}

}
