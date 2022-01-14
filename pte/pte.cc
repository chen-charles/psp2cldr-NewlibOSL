#include <psp2cldr/imp_provider.hpp>

#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include <psp2cldr/semaphore.hpp>

#include "thread.hpp"

std::recursive_mutex threads_lock;
std::unordered_map<uint32_t, std::shared_ptr<pte_thread>> threads;

uintptr_t tls_lr_page = 0;

#undef pte_osInit
DEFINE_VITA_IMP_SYM_EXPORT(pte_osInit)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    threads[ctx->thread.tid()] = std::make_shared<pte_thread>();
    tls_lr_page = ctx->thread.tls.alloc();

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
    if (new_stacksz < 0x200000)
        new_stacksz = 0x200000;
    uint32_t new_priority = PARAM_2;
    uint32_t argv = PARAM_3;
    uint32_t pHandleOut = PARAM_4x(4);

    uint32_t sp_base = ctx->coord.mmap(0, new_stacksz);
    uint32_t sp = sp_base + new_stacksz;
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
            std::cerr << "stack corruption during thread init routines" << std::endl;
            HANDLER_RETURN(1);
        }

        succ_counter++;
    }
    if (succ_counter != ctx->load.thread_init_routines.size())
    {
        std::cerr << "thread init routines failed" << std::endl;
        HANDLER_RETURN(2);
    }

    (*thread)[RegisterAccessProxy::Register::FP]->w(sp_base);
    (*thread)[RegisterAccessProxy::Register::SP]->w(sp_base + new_stacksz);
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

#undef pte_osThreadChangeSceArgvPriorToStart
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadChangeSceArgvPriorToStart)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    std::shared_ptr<pte_thread> thread;
    {
        std::lock_guard guard{threads_lock};
        thread = threads.at(key);
    }

    int ret_val = 1;
    {
        std::unique_lock guard{thread->access_lock};
        if (!thread->started)
        {
            (*(thread->thread))[RegisterAccessProxy::Register::R0]->w(PARAM_1);
            (*(thread->thread))[RegisterAccessProxy::Register::R1]->w(PARAM_2);
            ret_val = 0;
        }
    }

    TARGET_RETURN(ret_val);
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
        auto stack_base = (*thread)[RegisterAccessProxy::Register::FP]->r();
        auto stack_sz = (*thread)[RegisterAccessProxy::Register::SP]->r() - stack_base;
        auto lr_page = (*thread)[RegisterAccessProxy::Register::LR]->r();

        thread->tls.set(tls_lr_page, lr_page);

        thread->start((*thread)[RegisterAccessProxy::Register::IP]->r(), (*thread)[RegisterAccessProxy::Register::LR]->r());
        std::shared_ptr<pte_thread> pte_thread_data;
        {
            std::lock_guard guard{threads_lock};
            pte_thread_data = threads.at(thread->tid());
        }
        {
            std::unique_lock guard{pte_thread_data->access_lock};
            pte_thread_data->started = true;
        }

        std::thread([stack_base, stack_sz, lr_page](ExecutionCoordinator *coord, std::weak_ptr<ExecutionThread> weak, LoadContext *load)
                    {
                        if (auto thread = weak.lock())
                        {
                            thread->join();

                            std::shared_ptr<pte_thread> pte_thread_data;
                            {
                                std::lock_guard guard{threads_lock};
                                pte_thread_data = threads.at(thread->tid());
                                if (pte_thread_data->delete_on_terminate)
                                {
                                    threads.erase(thread->tid());
                                }
                            }
                            {
                                std::unique_lock guard{pte_thread_data->access_lock};
                                pte_thread_data->terminated = true;
                            }
                            {
                                std::shared_lock guard{pte_thread_data->access_lock};
                                pte_thread_data->on_termination.broadcast(pte_thread_data.get());
                            }

                            if (thread->state() != ExecutionThread::THREAD_EXECUTION_STATE::RESTARTABLE)
                            {
                                // did not exit gracefully, nothing to do
                                return;
                            }

                            uint32_t sp = stack_base + stack_sz;
                            uint32_t lr = lr_page; // assuming UNTIL_POINT_HIT

                            size_t succ_counter = 0;
                            for (auto it = load->thread_fini_routines.rbegin(); it != load->thread_fini_routines.rend(); it++)
                            {
                                auto &la = *it;
                                (*thread)[RegisterAccessProxy::Register::SP]->w(sp);
                                (*thread)[RegisterAccessProxy::Register::LR]->w(lr);

                                uint32_t result;
                                if (thread->start(la, lr) != ExecutionThread::THREAD_EXECUTION_RESULT::OK || (*thread).join(&result) != ExecutionThread::THREAD_EXECUTION_RESULT::STOP_UNTIL_POINT_HIT || result != 0)
                                    break;
                                if (sp != (*thread)[RegisterAccessProxy::Register::SP]->r())
                                {
                                    coord->panic(thread.get(), load, 0xff, "stack corruption");
                                }

                                succ_counter++;
                            }

                            if (succ_counter != load->thread_fini_routines.size())
                            {
                                coord->panic(thread.get(), load, 0xff, "thread fini routines failed");
                            }
                        }

                        coord->munmap(stack_base, stack_sz);
                        coord->munmap(lr_page, 0x1000);
                        coord->thread_destroy(weak); },
                    &ctx->coord, thread, &ctx->load)
            .detach();
    }
    else
        HANDLER_RETURN(1);

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

    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread.tls.get(tls_lr_page)); // gracefully exit so we can restart it from exit handling thread
    HANDLER_RETURN(0);
}

#undef pte_osThreadExitAndDelete
DEFINE_VITA_IMP_SYM_EXPORT(pte_osThreadExitAndDelete)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::lock_guard guard{threads_lock};
    threads.at(ctx->thread.tid())->delete_on_terminate = true;

    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread.tls.get(tls_lr_page));
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
        {
            std::unique_lock guard{thread->access_lock};
            thread->cancelled = true;
        }
        {
            std::shared_lock guard{thread->access_lock};
            thread->on_cancellation.broadcast(thread.get());
        }
    }
    else
        HANDLER_RETURN(1);

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
                std::cerr << key << " is not a valid thread handle" << std::endl;
                TARGET_RETURN(5);
                HANDLER_RETURN(0);
            }
        }

        if (thread)
        {
            bool needs_wait = false;
            MulticastDelegate<std::function<void(const pte_thread *thread)>>::Token *a = nullptr, *b = nullptr;

            semaphore waiter(0);
            bool done = false;
            {
                std::unique_lock guard(thread->access_lock);
                if (thread->terminated)
                {
                    result = 0;
                    done = true;
                }
                else
                {
                    b = &(thread->on_termination.add([&result, &waiter](const pte_thread *thread) -> void
                                                     {
                                                         result = 0;
                                                         waiter.release(); }));
                }
            }

            if (!done)
            {
                std::unique_lock guard(this_thread->access_lock);
                if (this_thread->cancelled)
                {
                    result = 4;
                    done = true;
                }
                else
                {
                    a = &(this_thread->on_cancellation.add([&result, &waiter](const pte_thread *thread) -> void
                                                           {
                                                               result = 4;
                                                               waiter.release(); }));
                }
            }

            if (!done)
            {
                waiter.acquire();
            }

            if (a)
            {
                a->invalidate();
            }

            if (b)
            {
                b->invalidate();
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

    const auto epoch = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();

    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(epoch).count();
    micros -= seconds * 1000000;

    uint32_t tv = PARAM_0;
    uint32_t tz = PARAM_1;

    if (tv)
    {
        // note: make sure the types here matches target newlib's configuration
        ctx->coord.proxy().w<uint64_t>(tv, seconds); // tv_seconds;
        ctx->coord.proxy().w<uint32_t>(tv + sizeof(uint64_t), (uint32_t)micros);
    }

    if (tz)
    {
        // obsolete
        ctx->coord.proxy().w<uint32_t>(tz, 0);
        ctx->coord.proxy().w<uint32_t>(tz + sizeof(uint32_t), 0);
    }

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}
