#include "ureactor/macros.h"
#include<iostream>
#include<type_traits>
#include<limits>
#include<string>

static_assert(sizeof(u8)==1 && std::is_unsigned_v<u8>);
static_assert(sizeof(u16)==2 && std::is_unsigned_v<u16>);
static_assert(sizeof(u32)==4 && std::is_unsigned_v<u32>);
static_assert(sizeof(u64)==8 && std::is_unsigned_v<u64>);
static_assert(sizeof(s8)==1 && std::is_signed_v<s8>);
static_assert(sizeof(s16)==2 && std::is_signed_v<s16>);
static_assert(sizeof(s32)==4 && std::is_signed_v<s32>);
static_assert(sizeof(s64)==8 && std::is_signed_v<s64>);
static_assert(std::numeric_limits<u8>::max()==MAX_U8);
static_assert(std::numeric_limits<u16>::max()==MAX_U16);
static_assert(std::numeric_limits<u32>::max()==MAX_U32);
static_assert(std::numeric_limits<u64>::max()==MAX_U64);
static_assert(INVALID8==MAX_U8);
static_assert(INVALID16==MAX_U16);
static_assert(INVALID32==MAX_U32);
static_assert(INVALID64==MAX_U64);

void test_types_and_constants()
{
    ASSERT_NOEFFECT(sizeof(u8)==1 && std::is_unsigned_v<u8>);
    ASSERT_NOEFFECT(sizeof(u16)==2 && std::is_unsigned_v<u16>);
    ASSERT_NOEFFECT(sizeof(u32)==4 && std::is_unsigned_v<u32>);
    ASSERT_NOEFFECT(sizeof(u64)==8 && std::is_unsigned_v<u64>);
    ASSERT_NOEFFECT(sizeof(s8)==1 && std::is_signed_v<s8>);
    ASSERT_NOEFFECT(sizeof(s16)==2 && std::is_signed_v<s16>);
    ASSERT_NOEFFECT(sizeof(s32)==4 && std::is_signed_v<s32>);
    ASSERT_NOEFFECT(sizeof(s64)==8 && std::is_signed_v<s64>);
    ASSERT_NOEFFECT(INVALID8==MAX_U8);
    ASSERT_NOEFFECT(INVALID16==MAX_U16);
    ASSERT_NOEFFECT(INVALID32==MAX_U32);
    ASSERT_NOEFFECT(INVALID64==MAX_U64);
    ASSERT_RETNONE(std::numeric_limits<u8>::max()==MAX_U8);
    ASSERT_RETNONE(std::numeric_limits<u16>::max()==MAX_U16);
    ASSERT_RETNONE_MSG(std::numeric_limits<u32>::max()==MAX_U32, "this is a test for u32 max value mismatch");
    ASSERT_RETNONE_MSG(std::numeric_limits<u64>::max()==MAX_U64, "this is a test for u64 max value mismatch");   

}

[[nodiscard]] int test_assert_retval()
{
    ASSERT_RETVAL(false, -1);
    return 0;
}

[[nodiscard]] int test_assert_retval_msg()
{
    ASSERT_RETVAL_MSG(false, -2, "this is a test for assert retval msg");
    return 0;
}

void test_assert_macros()
{
    ASSERT_NOEFFECT(true);
    ASSERT_NOEFFECT_MSG(true, "this is a test for assert noeffect msg");
    int never_reached = 5;
    int i = 0;
    for(; i<10; ++i)
    {
        ASSERT_CONTINUE(i != never_reached);
    }
    std::cout << "A------------------------------------" << i << std::endl;

    for(i=0; i<10; ++i)
    {
        ASSERT_CONTINUE_MSG(i != never_reached, "loop index should not be " +std::to_string(i));
    }
    std::cout << "B------------------------------------" << i << std::endl;

    for(i=0; i<10; ++i)
    {
        ASSERT_BREAK(i != never_reached);
    }
    std::cout << "C------------------------------------" << i << std::endl;

    for(i=0; i<10; ++i)
    {
        ASSERT_BREAK_MSG(i != never_reached, "loop index should not be " +std::to_string(i));
    }
    std::cout << "D------------------------------------" << i << std::endl;
    
}

int main()
{
#ifndef NDEBUG
    std::cout << "Assertions enabled\n";
#else
    std::cout << "Failures are logged; fallback actions remain active\n";
#endif

    test_types_and_constants();
    test_assert_macros();
    auto retval1 = test_assert_retval();
    std::cout << "retval1: " << retval1 << std::endl;
    auto retval2 = test_assert_retval_msg();
    std::cout << "retval2: " << retval2 << std::endl;

    return EXIT_SUCCESS;
}