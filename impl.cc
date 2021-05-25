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

    if (increment < 0)
    {
        HANDLER_RETURN(1);
    }

    static const uint32_t size = 0x5000000; // 80 MB

    std::lock_guard guard{sbrk_lock};

    static uint32_t begin = ctx->coord.mmap(0, size);
    static uint32_t top = begin;

    if (top + increment < begin + size)
    {
        ctx->thread[RegisterAccessProxy::Register::R0]->w(top);
        top += increment;
    }
    else
    {
        std::cerr << "heap overflow detected!" << std::endl;
        ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    }
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _exit
DEFINE_VITA_IMP_SYM_EXPORT(_exit)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    std::cerr << "_exit called, shutting down" << std::endl;
    // ctx->coord.thread_stopall(0);
    return std::make_shared<HandlerResult>(1);
}

#undef _getpid
DEFINE_VITA_IMP_SYM_EXPORT(_getpid)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(42);
    return std::make_shared<HandlerResult>(0);
}

#undef _times
DEFINE_VITA_IMP_SYM_EXPORT(_times)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    TARGET_RETURN(-1);
    return std::make_shared<HandlerResult>(0);
}

#undef _wait
DEFINE_VITA_IMP_SYM_EXPORT(_wait)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _kill
DEFINE_VITA_IMP_SYM_EXPORT(_kill)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _execve
DEFINE_VITA_IMP_SYM_EXPORT(_execve)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
}

#undef _fork
DEFINE_VITA_IMP_SYM_EXPORT(_fork)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    ctx->thread[RegisterAccessProxy::Register::R0]->w(-1);
    ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
    return std::make_shared<HandlerResult>(0);
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
