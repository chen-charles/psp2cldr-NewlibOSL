#include <psp2cldr/imp_provider.hpp>

#include <psp2cldr/handle.hpp>

#include <mutex>

static HandleStorage<std::shared_ptr<std::timed_mutex>> mutexes(0x100, INT32_MAX);

#undef pte_osMutexCreate
DEFINE_VITA_IMP_SYM_EXPORT(pte_osMutexCreate)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t p_out = PARAM_0;

    OSL_HANDLE key = mutexes.alloc(std::make_shared<std::timed_mutex>());
    ctx->coord.proxy().w<uint32_t>(p_out, key);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osMutexDelete
DEFINE_VITA_IMP_SYM_EXPORT(pte_osMutexDelete)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t key = PARAM_0;

    mutexes.free(key);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osMutexLock
DEFINE_VITA_IMP_SYM_EXPORT(pte_osMutexLock)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;

    if (auto mut = mutexes[key])
    {
        mut->lock();
        TARGET_RETURN(0);
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}

#undef pte_osMutexTimedLock
DEFINE_VITA_IMP_SYM_EXPORT(pte_osMutexTimedLock)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;
    uint32_t timeoutMsecs = PARAM_1; // milliseconds

    if (auto mut = mutexes[key])
    {
        if (mut->try_lock_for(std::chrono::milliseconds(timeoutMsecs)))
            TARGET_RETURN(0);
        else
            TARGET_RETURN(3);
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}

#undef pte_osMutexUnlock
DEFINE_VITA_IMP_SYM_EXPORT(pte_osMutexUnlock)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    uint32_t key = PARAM_0;

    if (auto mut = mutexes[key])
    {
        mut->unlock();
        TARGET_RETURN(0);
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}
