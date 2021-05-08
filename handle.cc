#include "handle.hpp"

std::random_device HandleAllocator::rand_dev;
std::mt19937 HandleAllocator::generator;
std::mutex HandleAllocator::generator_lock;

OSL_HANDLE HandleAllocator::alloc()
{
    std::lock_guard<std::recursive_mutex> guard{m_lock};
    if (m_allocated.size() > (m_high - m_low) * 0.75)
        throw std::runtime_error("out of handle space, use a better implementation ...");

    OSL_HANDLE key;
    do
    {
        {
            std::lock_guard<std::mutex> generator_guard{generator_lock};
            key = m_distr(generator);
        }
    } while (m_allocated.count(key) != 0);
    m_allocated.insert(key);
    return key;
}

void HandleAllocator::free(OSL_HANDLE key)
{
    std::lock_guard<std::recursive_mutex> guard{m_lock};
    if (m_allocated.erase(key) == 0)
        throw std::invalid_argument("attempted to free a handle that was not allocated");
}

bool HandleAllocator::is_valid(OSL_HANDLE key) const
{
    std::lock_guard<std::recursive_mutex> guard{m_lock};
    return m_allocated.count(key);
}
