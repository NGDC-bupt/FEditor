#include "threadpool.h"

ThreadPool::ThreadPool(size_t numThreads, string n) : stop(false), tasksInProgress(0), name(n) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this, i] {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] {
                        return this->stop.load() || !this->tasks.empty();
                    });

                    if (this->stop.load() && this->tasks.empty()) {
//                        cout << "byebye! im " << i << "in" << name << endl;
                        return;
                    }


                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                    ++this->tasksInProgress;
                }

                task();

                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    --this->tasksInProgress;
                    if (this->tasksInProgress == 0 && this->tasks.empty()) {
                        this->finishedCondition.notify_all();
                    }
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

//template<class F, class... Args>
//void ThreadPool::enqueue(F&& f, Args&&... args) {
//    auto tasks = std::make_shared<std::packaged_task<void()>>(
//            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
//    );
//
//    {
//        std::unique_lock<std::mutex> lock(queueMutex);
//        tasks.emplace([tasks]() { (*tasks)(); });
//    }
//    condition.notify_one();
//}

void ThreadPool::shutdown() {
    stop.store(true);
    condition.notify_all();
    for (std::thread &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queueMutex);
    finishedCondition.wait(lock, [this] {
//        cout << "tasks left" << this->tasks.size() << endl;
        return this->tasksInProgress == 0 && this->tasks.empty();
    });
}