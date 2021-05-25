#include <psp2cldr/imp_provider.hpp>

#include <mutex>

static std::recursive_mutex malloc_lock;

#undef __malloc_lock
DEFINE_VITA_IMP_SYM_EXPORT(__malloc_lock)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    malloc_lock.lock();

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef __malloc_unlock
DEFINE_VITA_IMP_SYM_EXPORT(__malloc_unlock)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);

    malloc_lock.unlock();

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}
