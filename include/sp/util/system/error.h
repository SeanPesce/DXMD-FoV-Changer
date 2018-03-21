/*
    CONTRIBUTORS:
        Sean Pesce

*/

#pragma once

#ifndef SP_UTILITY_SYSTEM_ERROR_H_
#define SP_UTILITY_SYSTEM_ERROR_H_


#include "sp.h"
#include "util/system/error/error_code.h"
#include <cstdint>


__SP_NAMESPACE
namespace util {
namespace sys  {
namespace err  {



inline uint32_t get_error()
{
#ifdef    _WIN32
    return (uint32_t)GetLastError();
#else  // UNIX
    return (uint32_t)errno;
#endif // _WIN32
}


inline uint32_t set_error(uint32_t err)
{
#ifdef    _WIN32
    SetLastError((DWORD)err);
#else  // UNIX
    errno = (int)err;
#endif // _WIN32
    return err;
}



} // namespace err
} // namespace sys
} // namespace util
__SP_NAMESPACE_CLOSE // namespace sp

#endif // SP_UTILITY_SYSTEM_ERROR_H_
