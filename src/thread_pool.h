#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <future>

#include "threadsafe_queue.h"

// taken from C++ Concurrency in action by A.Williams 
class FunctionWrapper
{
	struct impl_base
	{
		virtual void call()=0;
		virtual ~impl_base(){}
	};
	std::unique_ptr<impl_base> impl;
	template<typename F>
	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)){}
		void call(){ f(); }
	};
public:
	FunctionWrapper() = default;
	FunctionWrapper(FunctionWrapper&& rhs) : impl(std::move(rhs.impl)) {}
	FunctionWrapper& operator=(FunctionWrapper&& rhs)
	{
		impl = std::move(rhs.impl);
		return *this;
	}
	FunctionWrapper(const FunctionWrapper& rhs) = delete;
	FunctionWrapper(FunctionWrapper&) = delete;
	FunctionWrapper& operator=(const FunctionWrapper& rhs) = delete;
	template<typename F>
	FunctionWrapper(F&& f) : impl(new impl_type<F>(std::move(f))){}
	void operator()(){ impl->call(); }
};
class JoinThreads
{
public:
	explicit JoinThreads(std::vector<std::thread>& threads_) : threads(threads_) {}
	~JoinThreads()
	{
		for (unsigned long i = 0; i < threads.size(); ++i)
		{
			if (threads[i].joinable())
				threads[i].join();
		}
	}
private:
	std::vector<std::thread>& threads;
};
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	template<typename FunctionType, typename... Args>
	decltype(auto) submit(FunctionType f, Args&&... args);
	void runPendingTask()
	{
		FunctionWrapper task;
		if(workQueue.try_pop(task))
			task();
		else
			std::this_thread::yield();
	}
private:
	void workerThread();

	std::atomic_bool done;
	ThreadSafeQueue<FunctionWrapper> workQueue;
	std::vector<std::thread> threads;
	JoinThreads joiner;
};
template <typename FunctionType, typename... Args>
decltype(auto) ThreadPool::submit(FunctionType f, Args&&... args)
{
	using return_t = decltype(f(args...));

	auto bind_func = std::bind(f, std::forward<Args>(args)...);
	std::packaged_task<return_t()> task(std::move(bind_func));
	std::future<return_t> fu(task.get_future());
  workQueue.push(std::move(task));
	return fu;
}
