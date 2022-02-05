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

#ifndef PTE_THREAD_H
#define PTE_THREAD_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include <psp2cldr/utility/delegate.hpp>

class ExecutionThread;

class pte_thread
{
  public:
    pte_thread(std::shared_ptr<ExecutionThread> thread = {}, uint32_t priority = 160)
        : thread(thread), priority(priority)
    {
    }
    ~pte_thread()
    {
    }

    std::shared_ptr<ExecutionThread> thread;

  public:
    // lock-free
    std::atomic<bool> delete_on_terminate{false};

  public:
    uint32_t priority;
    bool started{false};
    bool cancelled{false};
    bool terminated{false};

    MulticastDelegate<std::function<void(const pte_thread *thread)>> on_cancellation;
    MulticastDelegate<std::function<void(const pte_thread *thread)>> on_termination;

    std::shared_mutex access_lock;
};

extern std::unordered_map<uint32_t, std::shared_ptr<pte_thread>> threads;
extern std::recursive_mutex threads_lock;

#endif
