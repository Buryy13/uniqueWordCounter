#include <thread>
#include <atomic>
#include <vector>

#include "threadsafe_queue.h"

class JoinThreads
{
public:
	explicit JoinThreads(std::vector<std::thread>& threads_) : threads(threads_){}
	~JoinThreads()
	{
		for(unsigned long i = 0; i < threads.size(); ++i)
		{
			if(threads[i].joinable)
				threads[i].join();
		}
	}
private:
	std::vector<std::thread> threads;
};
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	template<typename FunctionType>
	void submit(FunctionType f);

private:
	void workerThread();

	std::atomic_bool done;
	ThreadSafeQueue<std::function<void()>> workQueue;
	std::vector<std::thread> threads;
	JoinThreads joiner;
};
ThreadPool::ThreadPool() : done(false), joiner(threads)
{
	const unsigned threadCount = std::thread::hardware_concurrency();
	try
	{
		for(unsigned i = 0; i < threadCount; ++i)
		{
			threads.emplace_back(&ThreadPool::workerThread, this);
		}
	}	
	catch(...)
	{
		done = true;
		throw;
	}
}
ThreadPool::~ThreadPool()
{
	done = true;
}
template <typename FunctionType>
void ThreadPool::submit(FunctionType f)
{
	workThread.push(std::function<void()>(f));
}
