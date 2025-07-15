#ifndef TP
#define TP 1
#pragma once
#include <iostream>
#include <functional>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
using namespace std;

class ThreadPool {
public:
    ThreadPool(size_t numThreads, string name = "inner");
    ~ThreadPool();

    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args);

    void shutdown();
    void wait();

private:
    string name;
    vector<std::thread> workers;
    queue<std::function<void()>> tasks;
    mutex queueMutex;
    condition_variable condition;
    condition_variable finishedCondition;
    atomic<bool> stop;
    atomic<int> tasksInProgress;
};
template<class F, class... Args>
void ThreadPool::enqueue(F&& f, Args&&... args) {
    auto task = std::make_shared<std::packaged_task<void()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
}

#endif