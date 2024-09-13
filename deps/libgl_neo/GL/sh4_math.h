// ---- sh4_math.h - SH7091 Math Module ----
//
// Version 1.0.8
//
// This file is part of the DreamHAL project, a hardware abstraction library
// primarily intended for use on the SH7091 found in hardware such as the SEGA
// Dreamcast game console.
//
// This math module is hereby released into the public domain in the hope that it
// may prove useful. Now go hit 60 fps! :)
//
// --Moopthehedgehog
//

#pragma once

#ifndef __GNUC__
#endif

#define asm __asm__

typedef struct {
  float z1;
  float z2;
  float z3;
  float z4;
} RETURN_VECTOR_STRUCT;

// Please do not call this function directly (notice the weird syntax); call
// MATH_Cross_Product() or MATH_Cross_Product_with_Mult() instead.
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT xMATH_do_Cross_Product(float x3, float zero, float x1, float y3, float x2, float x1_2, float y1, float y2)
{
  // FR4-FR11 are the regs that are passed in, in that order.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tx1 = x1;
  float tx2 = x2;
  float tx3 = x3;
  float tx1_2 = x1_2;

  float ty1 = y1;
  float ty2 = y2;
  float ty3 = y3;
  float tzero = zero;

  register float __x1 __asm__("fr6") = tx1; // in place
  register float __x2 __asm__("fr8") = tx2; // in place (fmov to fr2, negate fr2)
  register float __x3 __asm__("fr4") = tx3; // need to negate (fmov to fr1, negate fr4)

  register float __zero __asm__("fr5") = tzero; // in place
  register float __x1_2 __asm__("fr9") = tx1_2; // need to negate

  register float __y1 __asm__("fr10") = ty1;
  register float __y2 __asm__("fr11") = ty2;
  // no __y3 needed in this function

  register float __z1 __asm__("fr0") = tzero; // z1
  register float __z2 __asm__("fr1") = tzero; // z2
  register float __z3 __asm__("fr2") = ty3; // z3
  register float __c __asm__("fr3") = tzero; // c

  // This actually does a matrix transform to do the cross product.
  // It's this:
  //                   _    _       _            _
  //  [  0 -x3 x2 0 ] |  y1  |     | -x3y2 + x2y3 |
  //  [  x3 0 -x1 0 ] |  y2  |  =  |  x3y1 - x1y3 |
  //  [ -x2 x1 0  0 ] |  y3  |     | -x2y1 + x1y2 |
  //  [  0  0  0  0 ] |_ 0  _|     |_      0     _|
  //

  asm volatile (
    // zero out FR7. For some reason, if this is done in C after __z3 is set:
    // register float __y3 __asm__("fr7") = tzero;
    // then GCC will emit a spurious stack push (pushing FR12). So just zero it here.
    "fmov FR5, FR7\n\t"
    // set up back bank's FV0
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs)

    // Save FR12-FR15, which are supposed to be preserved across functions calls.
    // This stops them from getting clobbered and saves 4 stack pushes (memory accesses).
    "fmov DR12, XD12\n\t"
    "fmov DR14, XD14\n\t"

    "fmov DR10, XD0\n\t" // fmov 'y1' and 'y2' from FR10, FR11 into position (XF0, XF1)
    "fmov DR2, XD2\n\t" // fmov 'y3' and '0' from FR2, FR3 into position (XF2, XF3)

    // pair move zeros for some speed in setting up front bank for matrix
    "fmov DR0, DR10\n\t" // clear FR10, FR11
    "fmov DR0, DR12\n\t" // clear FR12, FR13
    "fmov DR0, DR14\n\t" // clear FR14, FR15
    "fschg\n\t" // switch back to single moves
    // prepare front bank for XMTRX
    "fneg FR9\n\t" // set up 'x1'
    "fmov FR8, FR2\n\t" // set up 'x2'
    "fneg FR2\n\t"
    "fmov FR4, FR1\n\t" // set up 'x3'
    "fneg FR4\n\t"
    // flip banks and matrix multiply
    "frchg\n\t"
    "ftrv XMTRX, FV0\n"
  : "+&w" (__z1), "+&f" (__z2), "+&f" (__z3), "+&f" (__c) // output (using FV0)
  : "f" (__x1), "f" (__x2), "f" (__x3), "f" (__y1), "f" (__y2), "f" (__zero), "f" (__x1_2) // inputs
  : "fr7" // clobbers (all of the float regs get clobbered, except for FR12-FR15 which were specially preserved)
  );

  RETURN_VECTOR_STRUCT output = {__z1, __z2, __z3, __c};
  return output;
}

// Plain cross product; does not use the bonus float multiply (c = 0 and a in the cross product matrix will be 0)
// This is a tiny bit faster than 'with_mult' (about 2 cycles faster)
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT MATH_Cross_Product(float x1, float x2, float x3, float y1, float y2, float y3)
{
  return xMATH_do_Cross_Product(x3, 0.0f, x1, y3, x2, x1, y1, y2);
}

// |x|
// This one works on ARM and x86, too!
static inline __attribute__((always_inline)) float MATH_fabs(float x)
{
  asm volatile ("fabs %[floatx]\n"
    : [floatx] "+f" (x) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
  );

  return x;
}

// 1/x
// (1.0f / sqrt(x)) ^ 2
// This is about 3x faster than fdiv!
static inline __attribute__((always_inline)) float MATH_Invert(float x)
{
  asm volatile ("fsrra %[one_div_sqrt]\n\t"
  "fmul %[one_div_sqrt], %[one_div_sqrt]\n"
  : [one_div_sqrt] "+f" (x) // outputs, "+" means r/w
  : // no inputs
  : // no clobbers
  );

  return x;
}

// 1/sqrt(x)
static inline __attribute__((always_inline)) float MATH_fsrra(float x)
{
  asm volatile ("fsrra %[one_div_sqrt]\n"
  : [one_div_sqrt] "+f" (x) // outputs, "+" means r/w
  : // no inputs
  : // no clobbers
  );

  return x;
}

// It's faster to do this than to do an fdiv. This takes half as many cycles!
// (~7 vs ~14) Only fdiv can do doubles, however.
// Of course, not having to divide at all is generally the best way to go. :P
static inline __attribute__((always_inline)) float MATH_Fast_Divide(float numerator, float denominator)
{
  asm volatile ("fsrra %[div_denom]\n\t"
  "fmul %[div_denom], %[div_denom]\n\t"
  "fmul %[div_numer], %[div_denom]\n"
  : [div_denom] "+&f" (denominator) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
  : [div_numer] "f" (numerator) // inputs
  : // clobbers
  );

  return denominator;
}

static inline __attribute__((always_inline)) float MATH_Slow_Divide(float numerator, float denominator)
{
  asm volatile (
  "fdiv %[div_denom], %[div_numer]\n"
  : [div_numer] "+&f" (numerator) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
  : [div_denom] "f" (denominator) // inputs
  : // clobbers
  );
  return numerator;
}

// a*b+c
static inline __attribute__((always_inline)) float MATH_fmac(float a, float b, float c)
{
  asm volatile ("fmac fr0, %[floatb], %[floatc]\n"
    : [floatc] "+f" (c) // outputs, "+" means r/w
    : "w" (a), [floatb] "f" (b) // inputs
    : // no clobbers
  );

  return c;
}
