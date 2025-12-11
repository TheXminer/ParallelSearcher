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
    
    while (true) {
        std::function<void()> task;

        //std::cout << "Iteration started" << std::endl;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [&] {
                //std::cout << "Thread sleep" << std::endl;
                return stop.load() || !tasks.empty();
                });

            if (stop.load() && tasks.empty())
            {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}