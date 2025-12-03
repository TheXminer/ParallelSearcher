#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threads)
{
	for (size_t i = 0; i < threads; ++i)
		workers.emplace_back(&ThreadPool::workerLoop, this);
}

ThreadPool::~ThreadPool()
{
	stopPool();
}

void ThreadPool::stopPool()
{
	stop.store(true);
	condition.notify_all();

	for (std::thread& worker : workers)
		if (worker.joinable())
			worker.join();
}

void ThreadPool::workerLoop()
{
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [&] {
                return stop.load() || !tasks.empty();
                });

            if (stop.load() && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}

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