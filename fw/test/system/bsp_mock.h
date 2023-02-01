/*
 * bsp_mock.h
 *
 *  Created on: Mar 2, 2019
 *      Author: kbrunham
 */

#ifndef INC_BSP_MOCK_H
#define INC_BSP_MOCK_H

#include "alt_types_mock.h"

#ifndef NO_SYSTEM_MOCK
// Define the system mock object
#include "system_mock.h"
#ifndef USE_SYSTEM_MOCK
#define USE_SYSTEM_MOCK
#endif
static alt_u32 __builtin_ldwio(void* src)
{
    return SYSTEM_MOCK::get()->get_mem_word(src);
}
static void __builtin_stwio(void* dst, alt_u32 data)
{
    SYSTEM_MOCK::get()->set_mem_word(dst, data);
}

// Define the asserts to use the system mock. This allows runtime control of abort() vs. throw
#define PFR_ASSERT(condition)                                          \
    if (!(condition))                                                  \
    {                                                                  \
        SYSTEM_MOCK::get()->throw_internal_error(                      \
            std::string("Assert: ") + #condition, __LINE__, __FILE__); \
    }
#define PFR_INTERNAL_ERROR(msg)                                            \
    {                                                                      \
        SYSTEM_MOCK::get()->throw_internal_error(msg, __LINE__, __FILE__); \
    }

#define PFR_INTERNAL_ERROR_VARG(format, ...) \
{ \
    char buffer ## __LINE__ [4096]; \
    ::snprintf(buffer ## __LINE__, sizeof(buffer ## __LINE__), format, __VA_ARGS__); \
    PFR_INTERNAL_ERROR(buffer ## __LINE__); \
}

#else
// No system mock. Just directly dereference the pointers to access host memory
static alt_u32 __builtin_ldwio(void* src)
{
    return *((alt_u32*)src);
}
static void __builtin_stwio(void* dst, alt_u32 data)
{
    alt_u32* dst_alt_u32 = (alt_u32*) dst;
    *dst_alt_u32 = data;
}

#endif  // NO_SYSTEM_MOCK

#endif  // INC_BSP_MOCK_H
