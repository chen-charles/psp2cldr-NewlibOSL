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
