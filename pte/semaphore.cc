/**
 * psp2cldr-NewlibOSL: psp2cldr Newlib OS Support Reference Implementation
 * Copyright (C) 2022 Jianye Chen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <psp2cldr/imp_provider.hpp>

#include <psp2cldr/utility/handle.hpp>
#include <psp2cldr/utility/semaphore.hpp>

#include "thread.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

static HandleStorage<std::shared_ptr<semaphore>> semaphores(0x100, INT32_MAX);

#undef pte_osSemaphoreCreate
DEFINE_VITA_IMP_SYM_EXPORT(pte_osSemaphoreCreate)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t init_val = PARAM_0;
    uint32_t p_out = PARAM_1;

    auto key = semaphores.alloc(std::make_shared<semaphore>(init_val));

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

            int return_val = 3;
            if (sema->cancellable_acquire_for(std::chrono::milliseconds(nTimeoutMs)))
            {
                return_val = 0;
            }
            else
            {
                return_val = 3;
            }
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

    if (auto sema = semaphores[key])
    {
        semaphores.free(key);
        TARGET_RETURN(0);
    }
    else
    {
        TARGET_RETURN(5);
    }

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

        std::unique_lock guard{thread->access_lock};

        if (thread->cancelled)
        {
            return_val = 4;
        }
        else
        {
            semaphore::canceler canceler;
            auto &token = thread->on_cancellation.add([&canceler](const pte_thread *thread) { canceler.cancel(); });
            canceler.on_completed.add([&token]() { token.invalidate(); });

            if (sema->cancellable_acquire_for(std::chrono::milliseconds(nTimeoutMs), guard, canceler))
            {
                return_val = 0;
            }
            else
            {
                if (canceler.is_cancelled())
                {
                    return_val = 4;
                }
                else
                {
                    return_val = 3;
                }
            }
        }
    }
    else
    {
        return_val = 5;
    }

    TARGET_RETURN(return_val);
    HANDLER_RETURN(0);
}
