#include <psp2cldr/imp_provider.hpp>

#include "../handle.hpp"

#include <mutex>
#include <unordered_map>
#include <unordered_set>

static HandleStorage<std::shared_ptr<uintptr_t>> os_storage(0x100, 0x7fffffff);

#undef pte_osTlsAlloc
DEFINE_VITA_IMP_SYM_EXPORT(pte_osTlsAlloc)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t p_out = PARAM_0;

    uintptr_t tls_key = ctx->thread.tls.alloc();
    OSL_HANDLE os_key = os_storage.alloc(std::make_shared<uintptr_t>(tls_key));

    ctx->coord.proxy().w<uint32_t>(p_out, os_key);

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}

#undef pte_osTlsSetValue
DEFINE_VITA_IMP_SYM_EXPORT(pte_osTlsSetValue)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t key = PARAM_0;
    uint32_t value = PARAM_1;

    if (auto tls_key = os_storage[key])
    {
        ctx->thread.tls.set(*tls_key, value);
        TARGET_RETURN(0);
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}

#undef pte_osTlsGetValue
DEFINE_VITA_IMP_SYM_EXPORT(pte_osTlsGetValue)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t key = PARAM_0;

    if (auto tls_key = os_storage[key])
    {
        TARGET_RETURN(ctx->thread.tls.get(*tls_key));
    }
    else
    {
        TARGET_RETURN(5);
    }

    HANDLER_RETURN(0);
}

#undef pte_osTlsFree
DEFINE_VITA_IMP_SYM_EXPORT(pte_osTlsFree)
{
    DECLARE_VITA_IMP_TYPE(FUNCTION);
    uint32_t key = PARAM_0;

    if (auto tls_key = os_storage[key])
    {
        ctx->thread.tls.free(*tls_key);
        os_storage.free(key);
    }
    else
    {
        HANDLER_RETURN(1);
    }

    TARGET_RETURN(0);
    HANDLER_RETURN(0);
}
