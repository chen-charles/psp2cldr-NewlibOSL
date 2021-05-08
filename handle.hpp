#ifndef OSL_HANDLE_H
#define OSL_HANDLE_H

#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

typedef uint32_t OSL_HANDLE;

// MT-safe
class HandleAllocator
{
    static std::random_device rand_dev;
    static std::mt19937 generator;
    static std::mutex generator_lock;

public:
    // [valid_low, valid_high]
    HandleAllocator(OSL_HANDLE low, OSL_HANDLE high) : m_low(low), m_high(high), m_distr(low, high)
    {
        if (m_high < m_low || m_high - m_low < 100)
            throw std::invalid_argument("handler space is too small");
    }

    virtual ~HandleAllocator() {}

public:
    virtual bool is_valid(OSL_HANDLE) const;
    virtual OSL_HANDLE alloc();
    virtual void free(OSL_HANDLE);

protected:
    OSL_HANDLE m_low;
    OSL_HANDLE m_high;

    mutable std::recursive_mutex m_lock;
    std::unordered_set<OSL_HANDLE> m_allocated;

    std::uniform_int_distribution<uint32_t> m_distr;
};

template <class T>
class HandleStorage : public HandleAllocator
{
public:
    HandleStorage(OSL_HANDLE low, OSL_HANDLE high) : HandleAllocator(low, high) {}
    virtual ~HandleStorage() {}

public:
    virtual OSL_HANDLE alloc(const T &t)
    {
        std::lock_guard<std::recursive_mutex> guard{m_lock};
        auto handle = HandleAllocator::alloc();
        m_storage[handle] = t;
        return handle;
    }

    virtual OSL_HANDLE alloc(T &&t)
    {
        std::lock_guard<std::recursive_mutex> guard{m_lock};
        auto handle = HandleAllocator::alloc();
        m_storage[handle] = std::move(t);
        return handle;
    }

    virtual OSL_HANDLE alloc()
    {
        std::lock_guard<std::recursive_mutex> guard{m_lock};
        auto handle = HandleAllocator::alloc();
        m_storage[handle];
        return handle;
    }

    virtual void free(OSL_HANDLE key)
    {
        std::lock_guard<std::recursive_mutex> guard{m_lock};
        m_storage.erase(key);
    }

public:
    T &operator[](OSL_HANDLE idx)
    {
        std::lock_guard<std::recursive_mutex> guard{m_lock};
        return m_storage[idx];
    }

    virtual T at(OSL_HANDLE key) const
    {
        std::lock_guard<std::recursive_mutex> guard{m_lock};
        return m_storage.at(key);
    }

protected:
    std::unordered_map<OSL_HANDLE, T> m_storage;
};

#endif
