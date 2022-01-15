/**
 * psp2cldr-NewlibOSL: psp2cldr Newlib OS Support Reference Implementation
 * Copyright (C) 2022 Jianye Chen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

static std::mutex sbrk_lock;
#undef _sbrk
DEFINE_VITA_IMP_SYM_EXPORT(_sbrk)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    int32_t increment = (int32_t)PARAM_0;

    static const uint32_t size = 0x28000000; // 640 MB

    std::lock_guard guard{sbrk_lock};

    static uint32_t begin = ctx->coord.mmap(0, size);
    static uint32_t top = begin;

    if (increment < 0)
    {
        int32_t decrement = -increment;
        if (top - begin >= decrement)
        {
            uint32_t prev_top = top;
            top -= decrement;

            TARGET_RETURN(prev_top);
            HANDLER_RETURN(0);
        }
        else
        {
            HANDLER_RETURN(1);
        }
    }

    if (top + increment < begin + size)
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(top);
        TARGET_RETURN(top);
        top += increment;
    }
    else
    {
        std::cerr << "ran out of heap" << std::endl;
        TARGET_RETURN(-1);
    }
    HANDLER_RETURN(0);
}

#undef _exit
DEFINE_VITA_IMP_SYM_EXPORT(_exit)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    HANDLER_RUNTIME_EXCEPTION("_exit called, shutting down");
}

#undef _getpid
DEFINE_VITA_IMP_SYM_EXPORT(_getpid)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(42);
    HANDLER_RETURN(0);
}

#undef _times
DEFINE_VITA_IMP_SYM_EXPORT(_times)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(-1);
    HANDLER_RETURN(0);
}

#undef _wait
DEFINE_VITA_IMP_SYM_EXPORT(_wait)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(-1);
    HANDLER_RETURN(0);
}

#undef _kill
DEFINE_VITA_IMP_SYM_EXPORT(_kill)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(-1);
    HANDLER_RETURN(0);
}

#undef _execve
DEFINE_VITA_IMP_SYM_EXPORT(_execve)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(-1);
    HANDLER_RETURN(0);
}

#undef _fork
DEFINE_VITA_IMP_SYM_EXPORT(_fork)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(-1);
    HANDLER_RETURN(0);
}

#include <chrono>
#include <thread>

#undef sleep
DEFINE_VITA_IMP_SYM_EXPORT(sleep)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::this_thread::sleep_for(std::chrono::seconds(PARAM_0));
    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef usleep
DEFINE_VITA_IMP_SYM_EXPORT(usleep)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::this_thread::sleep_for(std::chrono::microseconds(PARAM_0));
    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef nanosleep
DEFINE_VITA_IMP_SYM_EXPORT(nanosleep)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::this_thread::sleep_for(std::chrono::microseconds(PARAM_0));
    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}
