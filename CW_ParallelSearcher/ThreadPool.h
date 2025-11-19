#pragma once
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool
{
private:
	std::vector<std::thread> workers;
	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;

	std::vector<std::function<void()>> tasks;

public:
	ThreadPool(size_t threads = std::thread::hardware_concurrency());
	~ThreadPool();
	template<typename F, typename... Args>
	auto enqueue(F&& f, Args&&... args)->std::future<typename std::result_of<F(Args...)>::type>;
	void stopPool();
};

