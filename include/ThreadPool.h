#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>
#include <algorithm>

namespace GameForge {

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = 0) : stop(false) {
        if (threadCount == 0) {
            unsigned int hw = std::thread::hardware_concurrency();
            threadCount = hw > 0 ? hw : 4;
        }

        for (size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->cv.wait(lock, [this]() {
                            return this->stop || !this->tasks.empty();
                        });

                        if (this->stop && this->tasks.empty()) {
                            return;
                        }

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void shutdown() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) return;
            stop = true;
        }
        cv.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        workers.clear();
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) {
                throw std::runtime_error("enqueue chamado em ThreadPool finalizado");
            }
            tasks.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return res;
    }

    // Executa fatias de um range [start, end) em paralelo através dos threads
    template<typename Func>
    void parallel_for(size_t start, size_t end, Func func) {
        size_t total = end - start;
        if (total == 0) return;

        size_t numThreads = workers.empty() ? 1 : workers.size();
        if (total < numThreads || numThreads == 1) {
            // Executa sequencialmente se o volume for muito pequeno
            for (size_t i = start; i < end; ++i) {
                func(i);
            }
            return;
        }

        size_t chunkSize = (total + numThreads - 1) / numThreads;
        std::vector<std::future<void>> futures;

        for (size_t t = 0; t < numThreads; ++t) {
            size_t chunkStart = start + t * chunkSize;
            size_t chunkEnd = std::min(chunkStart + chunkSize, end);

            if (chunkStart >= end) break;

            futures.push_back(enqueue([chunkStart, chunkEnd, func]() {
                for (size_t i = chunkStart; i < chunkEnd; ++i) {
                    func(i);
                }
            }));
        }

        for (auto& fut : futures) {
            fut.get();
        }
    }

    size_t getThreadCount() const noexcept { return workers.size(); }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> stop;
};

} // namespace GameForge
