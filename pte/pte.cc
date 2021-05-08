#include <psp2cldr/imp_provider.hpp>

#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "thread.hpp"

std::recursive_mutex threads_lock;
std::unordered_map<uint32_t, std::shared_ptr<pte_thread>> threads;

#undef pte_osInit
DEFINE_VITA_IMP_SYM_EXPORT(pte_osInit)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    threads[ctx->thread.tid()] = std::make_shared<pte_thread>();

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

// #undef _pthread_stack_default_user
// DEFINE_VITA_IMP_SYM_EXPORT(_pthread_stack_default_user)
// {
//     DECLARE_VITA_IMP_TYPE(VARIABLE);
//     uint32_t p_var = ctx->thread[RegisterAccessProxy::Register::PC]->r();
//     ctx->coord.proxy().w<uint32_t>(p_var, 0x200000);
//     TARGET_RETURN(0);
//     HANDLER_RETURN(0);
// }

#undef pte_osThreadCreate
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadCreate)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t new_pc = PARAM_0;
    uint32_t new_stacksz = PARAM_1;
    if (new_stacksz < 0x1000)
        new_stacksz = 0x1000;
    uint32_t new_priority = PARAM_2;
    uint32_t argv = PARAM_3;
    uint32_t pHandleOut = PARAM_4x(4);

    static const size_t stack_sz = 0x4000;
    uint32_t sp_base = ctx->coord.mmap(0, stack_sz);
    uint32_t sp = sp_base + stack_sz;
    uint32_t lr = ctx->coord.mmap(0, 0x1000);

    size_t succ_counter = 0;
    auto thread = ctx->coord.thread_create();
    for (auto &la : ctx->load.thread_init_routines)
    {
        (*thread)[RegisterAccessProxy::Register::SP]->w(sp);
        (*thread)[RegisterAccessProxy::Register::LR]->w(lr);

        uint32_t result;
        if (thread->start(la, lr) != ExecutionThread::THREAD_EXECUTION_RESULT::OK || (*thread).join(&result) != ExecutionThread::THREAD_EXECUTION_RESULT::STOP_UNTIL_POINT_HIT)
            break;

        if (sp != (*thread)[RegisterAccessProxy::Register::SP]->r())
        {
            std::cout << "stack corruption during thread init routines" << std::endl;
            HANDLER_RETURN(1);
        }

        succ_counter++;
    }
    if (succ_counter != ctx->load.thread_init_routines.size())
    {
        std::cout << "thread init routines failed" << std::endl;
        HANDLER_RETURN(2);
    }
    ctx->coord.munmap(sp_base, stack_sz);

    (*thread)[RegisterAccessProxy::Register::SP]->w(ctx->coord.mmap(0, new_stacksz) + new_stacksz);
    (*thread)[RegisterAccessProxy::Register::LR]->w(lr);
    (*thread)[RegisterAccessProxy::Register::IP]->w(new_pc); // we will store this in IP, start thread will use this value as PC
    (*thread)[RegisterAccessProxy::Register::R0]->w(argv);

    {
        std::lock_guard guard{threads_lock};
        threads[thread->tid()] = std::make_shared<pte_thread>(thread, new_priority);
    }

    ctx->coord.proxy().w<uint32_t>(pHandleOut, thread->tid());

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osThreadStart
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadStart)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    std::shared_ptr<ExecutionThread> thread;
    {
        std::lock_guard guard{threads_lock};
        thread = threads.at(key)->thread;
    }

    if (thread)
    {
        thread->start((*thread)[RegisterAccessProxy::Register::IP]->r(), (*thread)[RegisterAccessProxy::Register::LR]->r());
        std::thread([](ExecutionCoordinator *coord, std::weak_ptr<ExecutionThread> thread)
                    {
                        if (auto p = thread.lock())
                        {
                            p->join();
                        }
                        coord->thread_destroy(thread);
                    },
                    &ctx->coord, thread)
            .detach();
    }

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osThreadGetHandle
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadGetHandle)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(ctx->thread.tid());
    HANDLER_RETURN(0);
}

#undef pte_osThreadExit
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadExit)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread.stop();

    // target function must not return
    HANDLER_RETURN(0);
}

#undef pte_osThreadExitAndDelete
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadExitAndDelete)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread.stop();

    std::lock_guard guard{threads_lock};
    threads.erase(ctx->thread.tid());

    HANDLER_RETURN(0);
}

#undef pte_osThreadGetPriority
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadGetPriority)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;

    TARGET_RETURN(threads.at(key)->priority);
    HANDLER_RETURN(0);
}

#undef pte_osThreadSetPriority
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadSetPriority)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    uint32_t new_prio = PARAM_1;

    threads.at(key)->priority = new_prio;

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

std::mutex cancelled_map_lock;
std::unordered_map<uint32_t, bool> cancelled_map;

#undef pte_osThreadCancel
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadCancel)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    std::shared_ptr<pte_thread> thread;
    {
        std::lock_guard guard{threads_lock};
        thread = threads.at(key);
    }

    if (thread)
    {
        thread->cancelled = true;
    }

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osThreadCheckCancel
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadCheckCancel)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;

    std::lock_guard guard{threads_lock};
    if (threads.at(key)->cancelled)
        TARGET_RETURN(4);
    else
        TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osThreadSleep
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadSleep)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::this_thread::sleep_for(std::chrono::milliseconds(PARAM_0));

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osThreadWaitForEnd
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadWaitForEnd)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    uint32_t result = 0;
    {
        std::shared_ptr<pte_thread> thread;
        std::shared_ptr<pte_thread> this_thread;
        {
            std::lock_guard guard{threads_lock};
            try
            {
                thread = threads.at(key);
                this_thread = threads.at(ctx->thread.tid());
            }
            catch (const std::out_of_range &)
            {
                std::cout << key << " is not a valid thread handle" << std::endl;
                TARGET_RETURN(5);
                HANDLER_RETURN(0);
            }
        }

        if (thread)
        {
            // this sucks

            while (1)
            {
                if (thread->thread->state() != ExecutionThread::THREAD_EXECUTION_STATE::RUNNING)
                {
                    result = 0;
                    break;
                }

                if (this_thread->cancelled)
                {
                    result = 4;
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
        {
            result = 5;
        }
    }

    TARGET_RETURN(result);
    HANDLER_RETURN(0);
}

#undef pte_osThreadDelete
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadDelete)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::lock_guard guard{threads_lock};
    threads.erase(PARAM_0);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef _gettimeofday
DEFINE_VITA_IMP_SYM_EXPORT(_gettimeofday)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    auto tp = std::chrono::system_clock::now();
    auto dtn = tp.time_since_epoch();
    auto seconds = dtn.count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;

    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(dtn).count();
    micros -= seconds * 1000000;
    assert(micros <= 0xffffffff);

    uint32_t tv = PARAM_0;
    uint32_t tz = PARAM_1;

    ctx->coord.proxy().w<uint64_t>(tv, seconds); // tv_seconds;
    ctx->coord.proxy().w<uint32_t>(tv + sizeof(uint64_t), micros);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}
