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

	void push(const T& value);
	void wait_and_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	bool try_pop(T& value);
	std::shared_ptr<T> try_pop();
	bool empty() const;

private:
	std::queue<T> _queue;
	mutable std::mutex _mu;
	std::condition_variable _condVar;
};

template <typename T>
void ThreadSafeQueue<T>::push(const T& value)
{
	std::lock_guard<std::mutex> lm(_mu);
	_queue.push(value);
	_condVar.notify_one();
}

template <typename T>
void ThreadSafeQueue<T>::wait_and_pop(T& value)
{
	std::unique_lock<std::mutex> lm(_mu);
	_condVar.wait(lm, [this]() {return !_queue.empty(); });
	value = _queue.front();
	_queue.pop();
}

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue& rhs)
{
	std::lock_guard<std::mutex> lm(rhs._mu);
	_queue = rhs._queue;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop()
{
	std::unique_lock<std::mutex> lm(_mu);
	_condVar.wait(lm, [this]() {return !_queue.empty(); });
	std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
	_queue.pop();
	return res;
}

template <typename T>
bool ThreadSafeQueue<T>::try_pop(T& value)
{
	std::lock_guard<std::mutex> lm(_mu);
	if (_queue.empty()) return false;
	value = _queue.front();
	_queue.pop();
	return true;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::try_pop()
{
	std::lock_guard<std::mutex> lm(_mu);
	if (_queue.empty()) return std::shared_ptr<T>();
	std::shared_ptr<T> res = std::make_shared<T>(_queue.front());
	_queue.pop();
	return res;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const
{
	std::lock_guard<std::mutex> lm(_mu);
	return _queue.empty();
}