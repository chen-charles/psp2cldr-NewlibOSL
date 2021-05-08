#include <psp2cldr/imp_provider.hpp>

#include <iostream>
#include <sstream>
#include <string>

#undef _sbrk
DEFINE_VITA_IMP_SYM_EXPORT(_sbrk)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    auto increment = PARAM_0;

    static const uint32_t size = 0x5000000; // 80 MB
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

    std::cout << "_exit called, shutting down" << std::endl;
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

// #undef __register_frame_info
// DEFINE_VITA_IMP_SYM_EXPORT(__register_frame_info)
// {
//     DECLARE_VITA_IMP_TYPE(FUNCTION);

//     ctx->thread[RegisterAccessProxy::Register::PC]->w(ctx->thread[RegisterAccessProxy::Register::LR]->r());
//     return std::make_shared<HandlerResult>(0);
// }

// #undef __cxa_thread_atexit
// DEFINE_VITA_IMP_SYM_EXPORT(__cxa_thread_atexit)
// {
//     DECLARE_VITA_IMP_TYPE(FUNCTION);

//     TARGET_RETURN(0);
//     HANDLER_RETURN(0);
// }

// #undef __aeabi_atexit
// DEFINE_VITA_IMP_SYM_EXPORT(__aeabi_atexit)
// {
//     DECLARE_VITA_IMP_TYPE(FUNCTION);

//     TARGET_RETURN(0);
//     HANDLER_RETURN(0);
// }
