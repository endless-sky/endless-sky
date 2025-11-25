
#include "config.h"

#include "cpu_caps.h"

#if defined(_WIN32) && (defined(_M_ARM) || defined(_M_ARM64))
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef PF_ARM_NEON_INSTRUCTIONS_AVAILABLE
#define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE 19
#endif
#endif

#ifdef HAVE_INTRIN_H
#include <intrin.h>
#endif
#ifdef HAVE_CPUID_H
#include <cpuid.h>
#endif

#include <array>
#include <cctype>
#include <string>


int CPUCapFlags{0};

namespace {

#if defined(HAVE_GCC_GET_CPUID) \
    && (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
using reg_type = unsigned int;
inline std::array<reg_type,4> get_cpuid(unsigned int f)
{
    std::array<reg_type,4> ret{};
    __get_cpuid(f, &ret[0], &ret[1], &ret[2], &ret[3]);
    return ret;
}
#define CAN_GET_CPUID
#elif defined(HAVE_CPUID_INTRINSIC) \
    && (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
using reg_type = int;
inline std::array<reg_type,4> get_cpuid(unsigned int f)
{
    std::array<reg_type,4> ret{};
    (__cpuid)(ret.data(), f);
    return ret;
}
#define CAN_GET_CPUID
#endif

} // namespace

al::optional<CPUInfo> GetCPUInfo()
{
    CPUInfo ret;

#ifdef CAN_GET_CPUID
    auto cpuregs = get_cpuid(0);
    if(cpuregs[0] == 0)
        return al::nullopt;

    const reg_type maxfunc{cpuregs[0]};

    cpuregs = get_cpuid(0x80000000);
    const reg_type maxextfunc{cpuregs[0]};

    ret.mVendor.append(reinterpret_cast<char*>(&cpuregs[1]), 4);
    ret.mVendor.append(reinterpret_cast<char*>(&cpuregs[3]), 4);
    ret.mVendor.append(reinterpret_cast<char*>(&cpuregs[2]), 4);
    auto iter_end = std::remove(ret.mVendor.begin(), ret.mVendor.end(), '\0');
    iter_end = std::unique(ret.mVendor.begin(), iter_end,
        [](auto&& c0, auto&& c1) { return std::isspace(c0) && std::isspace(c1); });
    ret.mVendor.erase(iter_end, ret.mVendor.end());
    if(!ret.mVendor.empty() && std::isspace(ret.mVendor.back()))
        ret.mVendor.pop_back();
    if(!ret.mVendor.empty() && std::isspace(ret.mVendor.front()))
        ret.mVendor.erase(ret.mVendor.begin());

    if(maxextfunc >= 0x80000004)
    {
        cpuregs = get_cpuid(0x80000002);
        ret.mName.append(reinterpret_cast<char*>(cpuregs.data()), 16);
        cpuregs = get_cpuid(0x80000003);
        ret.mName.append(reinterpret_cast<char*>(cpuregs.data()), 16);
        cpuregs = get_cpuid(0x80000004);
        ret.mName.append(reinterpret_cast<char*>(cpuregs.data()), 16);
        iter_end = std::remove(ret.mName.begin(), ret.mName.end(), '\0');
        iter_end = std::unique(ret.mName.begin(), iter_end,
            [](auto&& c0, auto&& c1) { return std::isspace(c0) && std::isspace(c1); });
        ret.mName.erase(iter_end, ret.mName.end());
        if(!ret.mName.empty() && std::isspace(ret.mName.back()))
            ret.mName.pop_back();
        if(!ret.mName.empty() && std::isspace(ret.mName.front()))
            ret.mName.erase(ret.mName.begin());
    }

    if(maxfunc >= 1)
    {
        cpuregs = get_cpuid(1);
        if((cpuregs[3]&(1<<25)))
            ret.mCaps |= CPU_CAP_SSE;
        if((ret.mCaps&CPU_CAP_SSE) && (cpuregs[3]&(1<<26)))
            ret.mCaps |= CPU_CAP_SSE2;
        if((ret.mCaps&CPU_CAP_SSE2) && (cpuregs[2]&(1<<0)))
            ret.mCaps |= CPU_CAP_SSE3;
        if((ret.mCaps&CPU_CAP_SSE3) && (cpuregs[2]&(1<<19)))
            ret.mCaps |= CPU_CAP_SSE4_1;
    }

#else

    /* Assume support for whatever's supported if we can't check for it */
#if defined(HAVE_SSE4_1)
#warning "Assuming SSE 4.1 run-time support!"
    ret.mCaps |= CPU_CAP_SSE | CPU_CAP_SSE2 | CPU_CAP_SSE3 | CPU_CAP_SSE4_1;
#elif defined(HAVE_SSE3)
#warning "Assuming SSE 3 run-time support!"
    ret.mCaps |= CPU_CAP_SSE | CPU_CAP_SSE2 | CPU_CAP_SSE3;
#elif defined(HAVE_SSE2)
#warning "Assuming SSE 2 run-time support!"
    ret.mCaps |= CPU_CAP_SSE | CPU_CAP_SSE2;
#elif defined(HAVE_SSE)
#warning "Assuming SSE run-time support!"
    ret.mCaps |= CPU_CAP_SSE;
#endif
#endif /* CAN_GET_CPUID */

#ifdef HAVE_NEON
#ifdef __ARM_NEON
    ret.mCaps |= CPU_CAP_NEON;
#elif defined(_WIN32) && (defined(_M_ARM) || defined(_M_ARM64))
    if(IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE))
        ret.mCaps |= CPU_CAP_NEON;
#else
#warning "Assuming NEON run-time support!"
    ret.mCaps |= CPU_CAP_NEON;
#endif
#endif

    return al::make_optional(ret);
}
