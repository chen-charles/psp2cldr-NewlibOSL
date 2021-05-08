#ifndef PTE_THREAD_H
#define PTE_THREAD_H

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

class ExecutionThread;

class pte_thread
{
public:
    pte_thread(std::shared_ptr<ExecutionThread> thread = {}, uint32_t priority = 160) : thread(thread), priority(priority) {}
    ~pte_thread() {}

    std::shared_ptr<ExecutionThread> thread;

public:
    std::atomic<uint32_t> priority;
    std::atomic<bool> cancelled{false};
};

extern std::unordered_map<uint32_t, std::shared_ptr<pte_thread>> threads;
extern std::recursive_mutex threads_lock;

#endif
