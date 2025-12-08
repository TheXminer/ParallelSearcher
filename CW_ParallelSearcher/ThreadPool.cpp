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
    for (std::thread& worker : workers) {
        if (worker.joinable())
            worker.join();
	}
}

void ThreadPool::workerLoop()
{
    std::cout << "Thread created" << std::endl;
    
    while (true) {
        std::cout << "Task started" << std::endl;
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [&] {
                std::cout << "Thread sleep" << std::endl;
                return stop.load() || !tasks.empty();
                });

            if (stop.load() && tasks.empty())
            {
                std::cout << "Thread closed" << std::endl;
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();

        std::cout << "Task finished" << std::endl;
    }
}