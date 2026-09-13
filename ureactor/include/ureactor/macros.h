#ifndef UREACTOR_INCLUDE_UREACTOR_MACROS_H
#define UREACTOR_INCLUDE_UREACTOR_MACROS_H

#include <cassert>
#include <cstdint>
#include <iostream>

// Debug: print and abort. NDEBUG: print and let the caller handle failure.
// expr_text is a string, so the original condition is never evaluated here.
#define UREACTOR_DETAIL_FAILURE(expr_text, msg) \
    do { \
        std::cerr << "file: " << __FILE__ << ", line: " << __LINE__ \
                  << ", Assertion failed: " << (expr_text) \
                  << "\nMessage: " << (msg) << '\n'; \
        assert(false && "UReactor assertion failed; see diagnostic above"); \
    } while (0)

#define UREACTOR_ASSERT(x) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, ""); \
        } \
    } while (0)

#define UREACTOR_ASSERT_MSG(x, msg) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, msg); \
        } \
    } while (0)

#define ASSERT_RETVAL(x, val) ASSERT_RETVAL_MSG(x, val, "")
#define ASSERT_RETVAL_MSG(x, val, msg) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, msg); \
            return (val); \
        } \
    } while (0)

#define ASSERT_RETNONE(x) ASSERT_RETNONE_MSG(x, "")
#define ASSERT_RETNONE_MSG(x, msg) \
    do { \
        if (!(x)) [[unlikely]] { \
            UREACTOR_DETAIL_FAILURE(#x, msg); \
            return; \
        } \
    } while (0)

#define ASSERT_NOEFFECT(x) UREACTOR_ASSERT(x)
#define ASSERT_NOEFFECT_MSG(x, msg) UREACTOR_ASSERT_MSG(x, msg)

// Do not wrap these in do/while: break/continue must affect the caller's loop.
// The caller's semicolon completes the final expression statement.
#define ASSERT_CONTINUE(x) ASSERT_CONTINUE_MSG(x, "")
#define ASSERT_CONTINUE_MSG(x, msg) \
    if (!(x)) [[unlikely]] { \
        UREACTOR_DETAIL_FAILURE(#x, msg); \
        continue; \
    } else (void)0

#define ASSERT_BREAK(x) ASSERT_BREAK_MSG(x, "")
#define ASSERT_BREAK_MSG(x, msg) \
    if (!(x)) [[unlikely]] { \
        UREACTOR_DETAIL_FAILURE(#x, msg); \
        break; \
    } else (void)0

#define INVALID64 (~0ULL)
#define INVALID32 (0xFFFFFFFFU)
#define INVALID16 (0xFFFFU)
#define INVALID8 (0xFFU)
#define MAX_U8 (0xFFU)
#define MAX_U16 (0xFFFFU)
#define MAX_U32 (0xFFFFFFFFU)
#define MAX_U64 (~0ULL)

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

#endif
