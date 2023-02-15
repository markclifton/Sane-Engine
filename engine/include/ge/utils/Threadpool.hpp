#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#pragma warning( disable : 4996 ) // TODO: Fix this C++17 Deprecation Warning

namespace GE
{
    namespace Utils
    {
        class ThreadPool
        {
        public:
            ThreadPool(size_t);
            ~ThreadPool();

            template<class F, class... Args>
            auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
            {
                using return_type = typename std::invoke_result<F, Args...>::type;
                auto task = std::make_shared< std::packaged_task<return_type()> >(
                    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                    );

                std::future<return_type> res = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    if (stop) abort(); // ("enqueue on stopped ThreadPool\n");
                    tasks.emplace([task]() { (*task)(); });
                }
                condition.notify_one();
                return res;
            }

            inline bool isEmpty() { return tasks.size() == 0; }

        private:
            std::vector<std::thread> workers;
            std::queue<std::function<void()>> tasks;
            std::mutex queue_mutex;
            std::condition_variable condition;
            bool stop{ false };
        };
    }
}