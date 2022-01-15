#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <vector>

static const size_t nThreads = 30;

std::atomic<bool> aborted{false};
std::atomic<uint32_t> ctr{0};
static void __attribute__((constructor)) test_heap_mt_safety()
{
    std::vector<std::shared_ptr<std::thread>> threads;

    for (int i = 0; i < nThreads; i++)
        threads.push_back(std::make_shared<std::thread>([&]()
                                                        {
                                                            while (!aborted)
                                                            {
                                                                uint32_t val = ctr++;
                                                                auto p = new uint32_t;
                                                                if (!p)
                                                                    throw std::runtime_error("new failed");
                                                                *p = val;
                                                                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                                                if (*p != val)
                                                                    throw std::runtime_error("heap corruption");
                                                                delete p;
                                                            }
                                                        }));

    std::this_thread::sleep_for(std::chrono::seconds(30));
    aborted = true;
    for (auto &thread : threads)
    {
        thread->join();
    }
    threads.clear();
}
