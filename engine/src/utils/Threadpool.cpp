#include <ge/utils/Threadpool.hpp>

#if defined(NN_BUILD_TARGET_PLATFORM_NX)
#include <nn/os/os_Thread.h>
#endif

namespace GE
{
	namespace Utils
	{
        ThreadPool::ThreadPool(size_t threads)
        {
#if defined(NN_BUILD_TARGET_PLATFORM_NX)
            nn::os::SetThreadCoreMask(nn::os::GetCurrentThread(), 0, 1);
#endif
            workers.reserve(threads);
            for (size_t i = 0; i < threads; ++i)
            {
                workers.emplace_back([this, i] {
                    
#if defined(NN_BUILD_TARGET_PLATFORM_NX)
                    const nn::Bit64 g_CoreMask[] = { 1, 2, 4 };
                    nn::os::SetThreadCoreMask(nn::os::GetCurrentThread(), (i %2) + 1, g_CoreMask[(i % 2) + 1]);
#endif
                    for (;;)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock, [this] { 
                                return this->stop || !this->tasks.empty(); 
                            });

                            if (this->stop && this->tasks.empty()) return;

                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                    });
#if defined(NN_BUILD_TARGET_PLATFORM_NX)
                workers.back().Start();
#endif
            }
        }

        ThreadPool::~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }

            condition.notify_all();
            for (auto& worker : workers)
#ifdef _WIN32
                worker.join();
#else   
                worker.Stop();
#endif
        }
	}
}