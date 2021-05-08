#include <psp2cldr/imp_provider.hpp>

#include "../handle.hpp"
#include "thread.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

class semaphore
{
    std::mutex mutex_;
    std::condition_variable condition_;
    unsigned long count_ = 0; // Initialized as locked.

public:
    void release()
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        ++count_;
        condition_.notify_one();
    }

    void acquire()
    {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        while (!count_) // Handle spurious wake-ups.
            condition_.wait(lock);
        --count_;
    }

    bool try_acquire()
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (count_)
        {
            --count_;
            return true;
        }
        return false;
    }
};

static HandleStorage<std::shared_ptr<semaphore>> semaphores(0x100, 0x7fffffff);

#undef pte_osSemaphoreCreate
DEFINE_VITA_IMP_SYM_EXPORT(pte_osSemaphoreCreate)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t init_val = PARAM_0;
    uint32_t p_out = PARAM_1;

    auto key = semaphores.alloc(std::make_shared<semaphore>());

    for (int i = 0; i < init_val; i++)
        semaphores[key]->release();

    ctx->coord.proxy().w<uint32_t>(p_out, key);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osSemaphorePost
DEFINE_VITA_IMP_SYM_EXPORT(pte_osSemaphorePost)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    uint32_t amt = PARAM_1;

    if (auto sema = semaphores[key])
    {
        for (int i = 0; i < amt; i++)
            sema->release();
        TARGET_RETURN(0);
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}

#undef pte_osSemaphorePend
DEFINE_VITA_IMP_SYM_EXPORT(pte_osSemaphorePend)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    uint32_t pTimeout = PARAM_1;

    if (auto sema = semaphores[key])
    {
        if (pTimeout == 0)
        {
            sema->acquire();
            TARGET_RETURN(0);
        }
        else
        {
            uint32_t nTimeoutMs = ctx->coord.proxy().r<uint32_t>(pTimeout);
            std::chrono::milliseconds timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(nTimeoutMs);
            int return_val = 3;
            bool succ = false;
            while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) < timeout)
            {
                if (succ = sema->try_acquire())
                {
                    return_val = 0;
                    break;
                }
                else
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (!succ) // last trial
                if (succ = sema->try_acquire())
                    return_val = 0;

            TARGET_RETURN(return_val);
        }
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}

#undef pte_osSemaphoreDelete
DEFINE_VITA_IMP_SYM_EXPORT(pte_osSemaphoreDelete)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;

    semaphores.free(key);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osSemaphoreCancellablePend
DEFINE_VITA_IMP_SYM_EXPORT(pte_osSemaphoreCancellablePend)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    uint32_t pTimeout = PARAM_1;

    std::shared_ptr<pte_thread> thread;
    {
        std::lock_guard guard{threads_lock};
        thread = threads.at(ctx->thread.tid());
    }

    int return_val;
    if (auto sema = semaphores[key])
    {
        uint32_t nTimeoutMs;
        if (pTimeout == 0)
            nTimeoutMs = 0xffffffff;
        else
            nTimeoutMs = ctx->coord.proxy().r<uint32_t>(pTimeout);

        std::chrono::milliseconds timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(nTimeoutMs);

        return_val = 3;
        bool succ = false;
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) < timeout)
        {
            if (thread->cancelled)
            {
                return_val = 4;
                goto _cancelled;
            }

            if (succ = sema->try_acquire())
            {
                return_val = 0;
                break;
            }
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!succ) // last trial
            if (succ = sema->try_acquire())
                return_val = 0;

    _cancelled:;
    }
    else
    {
        return_val = 5;
    }

    TARGET_RETURN(return_val);
    HANDLER_RETURN(0);
}
