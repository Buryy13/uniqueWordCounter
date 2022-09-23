#include "thread_pool.h"

ThreadPool::ThreadPool() : done(false), joiner(threads)
{
  const unsigned threadCount = std::thread::hardware_concurrency();
  try
  {
    for (unsigned i = 0; i < threadCount; ++i)
    {
      threads.emplace_back(&ThreadPool::workerThread, this);
    }
  }
  catch (...)
  {
    done = true;
    throw;
  }
}
ThreadPool::~ThreadPool()
{
  done = true;
}
void ThreadPool::runPendingTask()
{
  FunctionWrapper task;
  if(workQueue.try_pop(task))
    task();
  else
    std::this_thread::yield();
}
void ThreadPool::workerThread()
{
  while(!done)
  {
    FunctionWrapper task;
    if(workQueue.try_pop(task))
      task();
    else
      std::this_thread::yield();
  }
}
