#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template <typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue() = default;
	~ThreadSafeQueue() = default;

	ThreadSafeQueue(const ThreadSafeQueue& rhs);

	void push(T value);
	void wait_and_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	bool try_pop(T& value);
	std::shared_ptr<T> try_pop();
	bool empty() const;

private:
	std::queue<T> queue;
	mutable std::mutex mu;
	std::condition_variable condVar;
};

template <typename T>
void ThreadSafeQueue<T>::push(T value)
{
	std::lock_guard<std::mutex> lm(mu);
	queue.push(std::move(value));
	condVar.notify_one();
}

template <typename T>
void ThreadSafeQueue<T>::wait_and_pop(T& value)
{
	std::unique_lock<std::mutex> lm(mu);
	condVar.wait(lm, [this]() {return !queue.empty(); });
	value = std::move(queue.front());
	queue.pop();
}

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue& rhs)
{
	std::lock_guard<std::mutex> lm(rhs.mu);
	queue = rhs.queue;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop()
{
	std::unique_lock<std::mutex> lm(mu);
	condVar.wait(lm, [this]() {return !queue.empty(); });
	std::shared_ptr<T> res(std::make_shared<T>(std::move(queue.front())));
	queue.pop();
	return res;
}

template <typename T>
bool ThreadSafeQueue<T>::try_pop(T& value)
{
	std::lock_guard<std::mutex> lm(mu);
	if (queue.empty()) return false;
	value = std::move(queue.front());
	queue.pop();
	return true;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::try_pop()
{
	std::lock_guard<std::mutex> lm(mu);
	if (queue.empty()) return std::shared_ptr<T>();
	std::shared_ptr<T> res(std::make_shared<T>(std::move(queue.front())));
	queue.pop();
	return res;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const
{
	std::lock_guard<std::mutex> lm(mu);
	return queue.empty();
}
