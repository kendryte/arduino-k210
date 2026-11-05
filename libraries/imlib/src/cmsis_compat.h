#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __STATIC_FORCEINLINE static inline __attribute__((__always_inline__))

/**
 * @brief definition to pack two 16 bit values.
 */
#define __PKHBT(ARG1, ARG2, ARG3)                                                                                              \
    ((((int32_t)(ARG1) << 0) & (int32_t)0x0000FFFF) | (((int32_t)(ARG2) << ARG3) & (int32_t)0xFFFF0000))
#define __PKHTB(ARG1, ARG2, ARG3)                                                                                              \
    ((((int32_t)(ARG1) << 0) & (int32_t)0xFFFF0000) | (((int32_t)(ARG2) >> ARG3) & (int32_t)0x0000FFFF))

/**
 * @brief 8-bit fractional data type in 1.7 format.
 */
typedef int8_t q7_t;

/**
 * @brief 16-bit fractional data type in 1.15 format.
 */
typedef int16_t q15_t;

/**
 * @brief 32-bit fractional data type in 1.31 format.
 */
typedef int32_t q31_t;

/**
 * @brief 64-bit fractional data type in 1.63 format.
 */
typedef int64_t q63_t;

/**
  \brief   Unsigned Saturate
  \details Saturates an unsigned value.
  \param [in]  value  Value to be saturated
  \param [in]    sat  Bit position to saturate to (0..31)
  \param [in]  shift  Right shift (0..31)
  \return             Saturated value
 */
__STATIC_FORCEINLINE uint32_t __USAT_ASR(int32_t val, uint32_t sat, uint32_t shift)
{
    val >>= shift & 0x1F;

    if (sat <= 31U) {
        const uint32_t max = ((1U << sat) - 1U);
        if (val > (int32_t)max) {
            return max;
        } else if (val < 0) {
            return 0U;
        }
    }
    return (uint32_t)val;
}

/**
  \brief   Unsigned Saturate
  \details Saturates two unsigned values.
  \param [in]  value  Values to be saturated
  \param [in]    sat  Bit position to saturate to (0..15)
  \return             Saturated value
 */
__STATIC_FORCEINLINE uint32_t __USAT16(int32_t val, uint32_t sat)
{
    if (sat <= 15U) {
        const uint32_t max   = ((1U << sat) - 1U);
        int32_t        valHi = val >> 16;
        if (valHi > (int32_t)max) {
            valHi = max;
        } else if (valHi < 0) {
            valHi = 0U;
        }
        int32_t valLo = (val << 16) >> 16;
        if (valLo > (int32_t)max) {
            valLo = max;
        } else if (valLo < 0) {
            valLo = 0U;
        }
        return (valHi << 16) | valLo;
    }
    return (uint32_t)val;
}

__STATIC_FORCEINLINE int32_t __SSAT(int32_t val, uint32_t sat)
{
    if ((sat >= 1U) && (sat <= 32U)) {
        const int32_t max = (int32_t)((1U << (sat - 1U)) - 1U);
        const int32_t min = -1 - max;
        if (val > max) {
            return max;
        } else if (val < min) {
            return min;
        }
    }
    return val;
}

__STATIC_FORCEINLINE uint32_t __USAT(int32_t val, uint32_t sat)
{
    if (sat <= 31U) {
        const uint32_t max = ((1U << sat) - 1U);
        if (val > (int32_t)max) {
            return max;
        } else if (val < 0) {
            return 0U;
        }
    }
    return (uint32_t)val;
}

/*
 * @brief C custom defined SMLAD
 */
__STATIC_FORCEINLINE uint32_t __SMLAD(uint32_t x, uint32_t y, uint32_t sum)
{
    return ((uint32_t)(((((q31_t)x << 16) >> 16) * (((q31_t)y << 16) >> 16)) + ((((q31_t)x) >> 16) * (((q31_t)y) >> 16))
                       + (((q31_t)sum))));
}

/**
  \brief   Reverse byte order (32 bit)
  \details Reverses the byte order in unsigned integer value. For example, 0x12345678 becomes 0x78563412.
  \param [in]    value  Value to reverse
  \return               Reversed value
 */
#define __REV(value) __builtin_bswap32(value)

/**
  \brief   Reverse byte order (16 bit)
  \details Reverses the byte order within each halfword of a word. For example, 0x12345678 becomes 0x34127856.
  \param [in]    value  Value to reverse
  \return               Reversed value
 */
#define __REV16(value) __ROR(__REV(value), 16)

/**
  \brief   Reverse byte order (16 bit)
  \details Reverses the byte order in a 16-bit value and returns the signed 16-bit result. For example, 0x0080 becomes 0x8000.
  \param [in]    value  Value to reverse
  \return               Reversed value
 */
#define __REVSH(value) (int16_t)__builtin_bswap16(value)

/**
  \brief   Rotate Right in unsigned value (32 bit)
  \details Rotate Right (immediate) provides the value of the contents of a register rotated by a variable number of bits.
  \param [in]    op1  Value to rotate
  \param [in]    op2  Number of Bits to rotate
  \return               Rotated value
 */
__STATIC_FORCEINLINE uint32_t __ROR(uint32_t op1, uint32_t op2)
{
    op2 %= 32U;
    if (op2 == 0U) {
        return op1;
    }
    return (op1 >> op2) | (op1 << (32U - op2));
}

/*
 * @brief C custom defined SMUADX
 */
__STATIC_FORCEINLINE uint32_t __SMUADX(uint32_t x, uint32_t y)
{
    return ((uint32_t)(((((q31_t)x << 16) >> 16) * (((q31_t)y) >> 16)) + ((((q31_t)x) >> 16) * (((q31_t)y << 16) >> 16))));
}

/*
 * @brief C custom defined SMUAD
 */
__STATIC_FORCEINLINE uint32_t __SMUAD(uint32_t x, uint32_t y)
{
    return ((uint32_t)(((((q31_t)x << 16) >> 16) * (((q31_t)y << 16) >> 16)) + ((((q31_t)x) >> 16) * (((q31_t)y) >> 16))));
}

#ifdef __cplusplus
}
#endif
