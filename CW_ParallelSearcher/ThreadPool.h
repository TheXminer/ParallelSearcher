#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(size_t threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::_Invoke_result_t<F, Args...>>;

    void stopPool();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;

    std::atomic<bool> stop{ false };

private:
    void workerLoop();
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<std::_Invoke_result_t<F, Args...>>
{
    using return_type = std::_Invoke_result_t<F, Args...>;

    auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = taskPtr->get_future();

    {
        std::unique_lock<std::mutex> lock(queueMutex);

        if (stop.load())
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([taskPtr]() { (*taskPtr)(); });
    }

    condition.notify_one();
    return res;
}