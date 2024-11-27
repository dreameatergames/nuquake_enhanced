// ---- sh4_math.h - SH7091 Math Module ----
//
// Version 1.0.3
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

// Notes:
// - GCC 4 users have a different return type for the fsca functions due to an
//  internal compiler error regarding complex numbers; no issue under GCC 9.2.0
// - Using -m4 instead of -m4-single-only completely breaks the matrix and
//  vector operations
// - Function inlining must be enabled and not blocked by compiler options such
//  as -ffunction-sections, as blocking inlining will result in significant
//  performance degradation for the vector and matrix functions employing a
//  RETURN_VECTOR_STRUCT return type. I have added compiler hints and attributes
//  "static inline __attribute__((always_inline))" to mitigate this, so in most
//  cases the functions should be inlined regardless. If in doubt, check the
//  compiler asm output!
//

#ifndef __SH4_MATH_H_
#define __SH4_MATH_H_

#define GNUC_FSCA_ERROR_VERSION 4

//
// Fast SH4 hardware math functions
//
//
// High-accuracy users beware, the fsrra functions have an error of +/- 2^-21
// per http://www.shared-ptr.com/sh_insns.html
//

//==============================================================================
// Definitions
//==============================================================================
//
// Structures, useful definitions, and reference comments
//

// Front matrix format:
//
//    FV0 FV4 FV8  FV12
//    --- --- ---  ----
//  [ fr0 fr4 fr8  fr12 ]
//  [ fr1 fr5 fr9  fr13 ]
//  [ fr2 fr6 fr10 fr14 ]
//  [ fr3 fr7 fr11 fr15 ]
//
// Back matrix, XMTRX, is similar, although it has no FVn vector groups:
//
//  [ xf0 xf4 xf8  xf12 ]
//  [ xf1 xf5 xf9  xf13 ]
//  [ xf2 xf6 xf10 xf14 ]
//  [ xf3 xf7 xf11 xf15 ]
//

typedef struct __attribute__((aligned(32))) {
  float fr0;
  float fr1;
  float fr2;
  float fr3;
  float fr4;
  float fr5;
  float fr6;
  float fr7;
  float fr8;
  float fr9;
  float fr10;
  float fr11;
  float fr12;
  float fr13;
  float fr14;
  float fr15;
} ALL_FLOATS_STRUCT;

// Return structs should be defined locally so that GCC optimizes them into
// register usage instead of memory accesses.
typedef struct {
  float z1;
  float z2;
  float z3;
  float z4;
} RETURN_VECTOR_STRUCT;

#if __GNUC__ <= GNUC_FSCA_ERROR_VERSION
typedef struct {
  float sine;
  float cosine;
} RETURN_FSCA_STRUCT;
#endif

// Identity Matrix
//
//    FV0 FV4 FV8 FV12
//    --- --- --- ----
//  [  1   0   0   0  ]
//  [  0   1   0   0  ]
//  [  0   0   1   0  ]
//  [  0   0   0   1  ]
//

static const ALL_FLOATS_STRUCT identity_matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

//==============================================================================
// Basic math functions
//==============================================================================
//
// The following functions are available.
// Please see their definitions for other usage info, otherwise they may not
// work for you.
//
/*
  // |x|
  float MATH_fabs(float x)

  // sqrt(x)
  float MATH_fsqrt(float x)

  // a*b+c
  float MATH_fmac(float a, float b, float c)

  // a*b-c
  float MATH_fmac_Dec(float a, float b, float c)
*/

// |x|
// This one works on ARM and x86, too!
static inline __attribute__((always_inline)) float MATH_fabs(float x)
{
  asm volatile ("FABS %[floatx]\n"
    : [floatx] "+f" (x) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
  );

  return x;
}

// sqrt(x)
// This one works on ARM and x86, too!
// NOTE: There is a much faster version (MATH_Fast_Sqrt()) in the fsrra section of
// this file. Chances are you probably want that one.
static inline __attribute__((always_inline)) float MATH_fsqrt(float x)
{
  asm volatile ("fsqrt %[floatx]\n"
    : [floatx] "+f" (x) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
  );

  return x;
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

// a*b-c
static inline __attribute__((always_inline)) float MATH_fmac_Dec(float a, float b, float c)
{
  asm volatile ("fneg %[floatc]\n\t"
    "fmac fr0, %[floatb], %[floatc]\n"
    : [floatc] "+&f" (c) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
    : "w" (a), [floatb] "f" (b) // inputs
    : // no clobbers
  );

  return c;
}

//==============================================================================
// Fun with fsrra, which does 1/sqrt(x) in one cycle
//==============================================================================
//
// Error is +/- 2^-21 per http://www.shared-ptr.com/sh_insns.html
//
// The following functions are available.
// Please see their definitions for other usage info, otherwise they may not
// work for you.
//
/*
  // 1/x
  float MATH_Invert(float x)

  // 1/sqrt(x)
  float MATH_fsrra(float x)

  // A faster divide than the 'fdiv' instruction
  float MATH_Fast_Divide(float numerator, float denominator)

  // A faster square root then the 'fsqrt' instruction
  float MATH_Fast_Sqrt(float x)
*/

// 1/x
// (1.0f / sqrt(x) ) ^ 2
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

// fast sqrt(x)
// Crazy thing: invert(fsrra(x)) is actually about 3x faster than fsqrt.
// Error is +/- 2^-21 per http://www.shared-ptr.com/sh_insns.html
static inline __attribute__((always_inline)) float MATH_Fast_Sqrt(float x)
{
  return MATH_Invert(MATH_fsrra(x));
}

//==============================================================================
// Fun with fsca, which does simultaneous sine and cosine in 3 cycles
//==============================================================================
//
// NOTE: GCC 4.7 has a bug that prevents it from working with fsca and complex
// numbers in m4-single-only mode, so GCC 4 users will get a RETURN_FSCA_STRUCT
// instead of a _Complex float. This may be much slower in some instances.
//
// VERY IMPORTANT USAGE INFORMATION (sine and cosine functions):
//
// Due to the nature in which the fsca instruction behaves, you MUST do the
// following in your code to get sine and cosine from these functions:
//
//  _Complex float sine_cosine = [Call the fsca function here]
//  float sine_value = __real__ sine_cosine;
//  float cosine_value = __imag__ sine_cosine;
//  Your output is now in sine_value and cosine_value.
//
// This is necessary because fsca outputs both sine and cosine simultaneously
// and uses a double register to do so. The fsca functions do not actually
// return a double--they return two floats--and using a complex float here is
// just a bit of hacking the C language to make GCC do something that's legal in
// assembly according to the SH4 calling convention (i.e. multiple return values
// stored in floating point registers FR0-FR3). This is better than using a
// struct of floats for optimization purposes--this will operate at peak
// performance even at -O0, whereas a struct will not be fast at low
// optimization levels due to memory accesses.
//
// Technically you may be able to use the complex return values as a complex
// number if you wanted to, but that's probably not what you're after and they'd
// be flipped anyways (in mathematical convention, sine is the imaginary part).
//

// Notes:
// - From http://www.shared-ptr.com/sh_insns.html:
//      The input angle is specified as a signed fraction in twos complement. The result of sin and cos is a single-precision floating-point number.
//      0x7FFFFFFF to 0x00000001: 360×2^15−360/2^16 to 360/2^16 degrees
//      0x00000000: 0 degree
//      0xFFFFFFFF to 0x80000000: −360/2^16 to −360×2^15 degrees
// - fsca format is 2^16 is 360 degrees, so a value of 1 is actually
//  1/182.044444444 of a degree
// - fsca does a %360 automatically for values over 360 degrees

// The following functions are available.
// Please see their definitions for other usage info, otherwise they may not
// work for you.
//
/*
  // For integer input in native fsca units (fastest)
  _Complex float MATH_fsca_Int(unsigned int input_int)

  // For integer input in degrees
  _Complex float MATH_fsca_Int_Deg(unsigned int input_int)

  // For integer input in radians
  _Complex float MATH_fsca_Int_Rad(unsigned int input_int)

  // For float input in native fsca units
  _Complex float MATH_fsca_Float(float input_float)

  // For float input in degrees
  _Complex float MATH_fsca_Float_Deg(float input_float)

  // For float input in radians
  _Complex float MATH_fsca_Float_Rad(float input_float)
*/

//------------------------------------------------------------------------------
#if __GNUC__ <= GNUC_FSCA_ERROR_VERSION
//------------------------------------------------------------------------------
//
// This set of fsca functions is specifically for old versions of GCC.
// See later for functions for newer versions of GCC.
//

//
// Integer input (faster)
//

// For int input, input_int is in native fsca units (fastest)
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Int(unsigned int input_int)
{
  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For int input, input_int is in degrees
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Int_Deg(unsigned int input_int)
{
  // normalize whole number input degrees to fsca format
  input_int = ((1527099483ULL * input_int) >> 23);

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For int input, input_int is in radians
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Int_Rad(unsigned int input_int)
{
  // normalize whole number input rads to fsca format
  input_int = ((2734261102ULL * input_int) >> 18);

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

//
// Float input (slower)
//

// For float input, input_float is in native fsca units
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Float(float input_float)
{
  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For float input, input_float is in degrees
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Float_Deg(float input_float)
{
  input_float *= 182.044444444f;

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

// For float input, input_float is in radians
static inline __attribute__((always_inline)) RETURN_FSCA_STRUCT MATH_fsca_Float_Rad(float input_float)
{
  input_float *= 10430.3783505f;

  register float __sine __asm__("fr0");
  register float __cosine __asm__("fr1");

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, DR0\n" // 3 cycle simultaneous sine/cosine
    : "=w" (__sine), "=f" (__cosine) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  RETURN_FSCA_STRUCT output = {__sine, __cosine};
  return output;
}

//------------------------------------------------------------------------------
#else
//------------------------------------------------------------------------------
//
// This set of fsca functions is specifically for newer versions of GCC. They
// work fine under GCC 9.2.0.
//

//
// Integer input (faster)
//

// For int input, input_int is in native fsca units (fastest)
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Int(unsigned int input_int)
{
  _Complex float output;

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For int input, input_int is in degrees
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Int_Deg(unsigned int input_int)
{
  // normalize whole number input degrees to fsca format
  input_int = ((1527099483ULL * input_int) >> 23);

  _Complex float output;

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For int input, input_int is in radians
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Int_Rad(unsigned int input_int)
{
  // normalize whole number input rads to fsca format
  input_int = ((2734261102ULL * input_int) >> 18);

  _Complex float output;

  asm volatile ("lds %[input_number], FPUL\n\t" // load int from register (1 cycle)
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "r" (input_int)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

//
// Float input (slower)
//

// For float input, input_float is in native fsca units
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Float(float input_float)
{
  _Complex float output;

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For float input, input_float is in degrees
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Float_Deg(float input_float)
{
  input_float *= 182.044444444f;

  _Complex float output;

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

// For float input, input_float is in radians
static inline __attribute__((always_inline)) _Complex float MATH_fsca_Float_Rad(float input_float)
{
  input_float *= 10430.3783505f;

  _Complex float output;

  asm volatile ("ftrc %[input_number], FPUL\n\t" // convert float to int. takes 3 cycles
    "fsca FPUL, %[out]\n" // 3 cycle simultaneous sine/cosine
    : [out] "=d" (output) // outputs
    : [input_number] "f" (input_float)  // inputs
    : "fpul" // clobbers
  );

  return output;
}

//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------

//==============================================================================
// Hardware vector and matrix operations
//==============================================================================
//
// These functions each have very specific usage instructions. Please be sure to
// read them before use or else they won't seem to work right!
//
// The following functions are available.
// Please see their definitions for important usage info, otherwise they may not
// work for you.
//
/*
  // Inner/dot product (4x1 vec . 4x1 vec = scalar)
  float MATH_fipr(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)

  // Cross product with bonus multiply (vec X vec = orthogonal vec, with an extra a*b=c)
  RETURN_VECTOR_STRUCT MATH_Cross_Product_with_Mult(float x1, float x2, float x3, float y1, float y2, float y3, float a, float b)

  // Cross product (vec X vec = orthogonal vec)
  RETURN_VECTOR_STRUCT MATH_Cross_Product(float x1, float x2, float x3, float y1, float y2, float y3)

  // Outer product (vec (X) vec = 4x4 matrix)
  void MATH_Outer_Product(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)

  // Matrix transform (4x4 matrix * 4x1 vec = 4x1 vec)
  RETURN_VECTOR_STRUCT MATH_Matrix_Transform(float x1, float x2, float x3, float x4)

  // 4x4 Matrix transpose (XMTRX^T)
  void MATH_Matrix_Transpose(void)

  // 4x4 Matrix product (XMTRX and one from memory)
  void MATH_Matrix_Product(ALL_FLOATS_STRUCT * front_matrix)

  // 4x4 Matrix product (two from memory)
  void MATH_Load_Matrix_Product(ALL_FLOATS_STRUCT * matrix1, ALL_FLOATS_STRUCT * matrix2)

  // Load 4x4 XMTRX from memory
  void MATH_Load_XMTRX(ALL_FLOATS_STRUCT * back_matrix)

  // Store 4x4 XMTRX to memory
  ALL_FLOATS_STRUCT * MATH_Store_XMTRX(ALL_FLOATS_STRUCT * destination)

  // Get 4x1 column vector from XMTRX
  RETURN_VECTOR_STRUCT MATH_Get_XMTRX_Vector(unsigned int which)

  // Get 2x2 matrix from XMTRX quadrant
  RETURN_VECTOR_STRUCT MATH_Get_XMTRX_2x2(unsigned int which)
*/

// Inner/dot product: vec . vec = scalar
//                       _    _
//                      |  y1  |
//  [ x1 x2 x3 x4 ]  .  |  y2  | = scalar
//                      |  y3  |
//                      |_ y4 _|
//
// SH4 calling convention states we get 8 float arguments. Perfect!
static inline __attribute__((always_inline)) float MATH_fipr(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)
{
  // FR4-FR11 are the regs that are passed in, aka vectors FV4 and FV8.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tx1 = x1;
  float tx2 = x2;
  float tx3 = x3;
  float tx4 = x4;

  float ty1 = y1;
  float ty2 = y2;
  float ty3 = y3;
  float ty4 = y4;

  // vector FV4
  register float __x1 __asm__("fr4") = tx1;
  register float __x2 __asm__("fr5") = tx2;
  register float __x3 __asm__("fr6") = tx3;
  register float __x4 __asm__("fr7") = tx4;

  // vector FV8
  register float __y1 __asm__("fr8") = ty1;
  register float __y2 __asm__("fr9") = ty2;
  register float __y3 __asm__("fr10") = ty3;
  register float __y4 __asm__("fr11") = ty4;

  // take care of all the floats in one fell swoop
  asm volatile ("fipr FV4, FV8\n"
  : "+f" (__y4) // output (gets written to FR11)
  : "f" (__x1), "f" (__x2), "f" (__x3), "f" (__x4), "f" (__y1), "f" (__y2), "f" (__y3) // inputs
  : // clobbers
  );

  return __y4;
}

// Cross product: vec X vec = orthogonal vec
//   _    _       _    _       _    _
//  |  x1  |     |  y1  |     |  z1  |
//  |  x2  |  X  |  y2  |  =  |  z2  |
//  |_ x3 _|     |_ y3 _|     |_ z3 _|
//
// With bonus multiply:
//
//      a     *     b      =      c
//
// IMPORTANT USAGE INFORMATION (cross product):
//
// Return vector struct maps as below to the above diagram:
//
//  typedef struct {
//   float z1;
//   float z2;
//   float z3;
//   float z4; // c is stored in z4, and c = a*b if using 'with mult' version (else c = 0)
// } RETURN_VECTOR_STRUCT;
//
//  For people familiar with the unit vector notation, z1 == 'i', z2 == 'j',
//  and z3 == 'k'.
//
// The cross product matrix will also be stored in XMTRX after this, so calling
// MATH_Matrix_Transform() on a vector after using this function will do a cross
// product with the same x1-x3 values and a multiply with the same 'a' value
// as used in this function. In this a situation, 'a' will be multiplied with
// the x4 parameter of MATH_Matrix_Transform(). a = 0 if not using the 'with mult'
// version of the cross product function.
//
// For reference, XMTRX will look like this:
//
//  [  0 -x3 x2 0 ]
//  [  x3 0 -x1 0 ]
//  [ -x2 x1 0  0 ]
//  [  0  0  0  a ] (<-- a = 0 if not using 'with mult')
//
// Similarly to how the sine and cosine functions use fsca and return 2 floats,
// the cross product functions actually return 4 floats. The first 3 are the
// cross product output, and the 4th is a*b. The SH4 only multiplies 4x4
// matrices with 4x1 vectors, which is why the output is like that--but it means
// we also get a bonus float multiplication while we do our cross product!
//

// Please do not call this function directly (notice the weird syntax); call
// MATH_Cross_Product() or MATH_Cross_Product_with_Mult() instead.
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT xMATH_do_Cross_Product_with_Mult(float x3, float a, float y3, float b, float x2, float x1, float y1, float y2)
{
  // FR4-FR11 are the regs that are passed in, in that order.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tx1 = x1;
  float tx2 = x2;
  float tx3 = x3;
  float ta = a;

  float ty1 = y1;
  float ty2 = y2;
  float ty3 = y3;
  float tb = b;

  register float __x1 __asm__("fr9") = tx1; // need to negate (need to move to fr6, then negate fr9)
  register float __x2 __asm__("fr8") = tx2; // in place for matrix (need to move to fr2 then negate fr2)
  register float __x3 __asm__("fr4") = tx3; // need to negate (move to fr1 first, then negate fr4)
  register float __a __asm__("fr5") = ta;

  register float __y1 __asm__("fr10") = ty1;
  register float __y2 __asm__("fr11") = ty2;
  register float __y3 __asm__("fr6") = ty3;
  register float __b __asm__("fr7") = tb;

  register float __z1 __asm__("fr0") = 0.0f; // z1
  register float __z2 __asm__("fr1") = 0.0f; // z2 (not moving x3 here yet since a double 0 is needed)
  register float __z3 __asm__("fr2") = tx2; // z3 (this handles putting x2 in fr2)
  register float __c __asm__("fr3") = 0.0f; // c

  // This actually does a matrix transform to do the cross product.
  // It's this:
  //                   _    _       _            _
  //  [  0 -x3 x2 0 ] |  y1  |     | -x3y2 + x2y3 |
  //  [  x3 0 -x1 0 ] |  y2  |  =  |  x3y1 - x1y3 |
  //  [ -x2 x1 0  0 ] |  y3  |     | -x2y1 + x1y2 |
  //  [  0  0  0  a ] |_ b  _|     |_      c     _|
  //

  asm volatile (
    // set up back bank's FV0
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs)

    // Save FR12-FR15, which are supposed to be preserved across functions calls.
    // This stops them from getting clobbered and saves 4 stack pushes (memory accesses).
    "fmov DR12, XD12\n\t"
    "fmov DR14, XD14\n\t"

    "fmov DR10, XD0\n\t" // fmov 'y1' and 'y2' from FR10, FR11 into position (XF0, XF1)
    "fmov DR6, XD2\n\t" // fmov 'y3' and 'b' from FR6, FR7 into position (XF2, XF3)

    // pair move zeros for some speed in setting up front bank for matrix
    "fmov DR0, DR10\n\t" // clear FR10, FR11
    "fmov DR0, DR12\n\t" // clear FR12, FR13
    "fschg\n\t" // switch back to single moves
    // prepare front bank for XMTRX
    "fmov FR5, FR15\n\t" // fmov 'a' into position
    "fmov FR0, FR14\n\t" // clear out FR14
    "fmov FR0, FR7\n\t" // clear out FR7
    "fmov FR0, FR5\n\t" // clear out FR5

    "fneg FR2\n\t" // set up 'x2'
    "fmov FR9, FR6\n\t" // set up 'x1'
    "fneg FR9\n\t"
    "fmov FR4, FR1\n\t" // set up 'x3'
    "fneg FR4\n\t"
    // flip banks and matrix multiply
    "frchg\n\t"
    "ftrv XMTRX, FV0\n"
  : "+&w" (__z1), "+&f" (__z2), "+&f" (__z3), "+&f" (__c) // output (using FV0)
  : "f" (__x1), "f" (__x2), "f" (__x3), "f" (__y1), "f" (__y2), "f" (__y3), "f" (__a), "f" (__b) // inputs
  : // clobbers (all of the float regs get clobbered, except for FR12-FR15 which were specially preserved)
  );

  RETURN_VECTOR_STRUCT output = {__z1, __z2, __z3, __c};
  return output;
}

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

//------------------------------------------------------------------------------
// Functions that wrap the xMATH_do_Cross_Product[_with_Mult]() functions to make
// it easier to organize parameters
//------------------------------------------------------------------------------

// Cross product with a bonus float multiply (c = a * b)
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT MATH_Cross_Product_with_Mult(float x1, float x2, float x3, float y1, float y2, float y3, float a, float b)
{
  return xMATH_do_Cross_Product_with_Mult(x3, a, y3, b, x2, x1, y1, y2);
}

// Plain cross product; does not use the bonus float multiply (c = 0 and a in the cross product matrix will be 0)
// This is a tiny bit faster than 'with_mult' (about 2 cycles faster)
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT MATH_Cross_Product(float x1, float x2, float x3, float y1, float y2, float y3)
{
  return xMATH_do_Cross_Product(x3, 0.0f, x1, y3, x2, x1, y1, y2);
}

// Outer product: vec (X) vec = matrix
//   _    _
//  |  x1  |
//  |  x2  |  (X)  [ y1 y2 y3 y4 ] = 4x4 matrix
//  |  x3  |
//  |_ x4 _|
//
// This returns the floats in the back bank (XF0-15), which are inaccessible
// outside of using frchg or paired-move fmov. It's meant to set up a matrix for
// use with other matrix functions. GCC also does not touch the XFn bank.
// This will also wipe out anything stored in the float registers, as it uses the
// whole FPU register file (all 32 of the float registers).
static inline __attribute__((always_inline)) void MATH_Outer_Product(float x1, float x2, float x3, float x4, float y1, float y2, float y3, float y4)
{
  // FR4-FR11 are the regs that are passed in, in that order.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tx1 = x1;
  float tx2 = x2;
  float tx3 = x3;
  float tx4 = x4;

  float ty1 = y1;
  float ty2 = y2;
  float ty3 = y3;
  float ty4 = y4;

  // vector FV4
  register float __x1 __asm__("fr4") = tx1;
  register float __x2 __asm__("fr5") = tx2;
  register float __x3 __asm__("fr6") = tx3;
  register float __x4 __asm__("fr7") = tx4;

  // vector FV8
  register float __y1 __asm__("fr8") = ty1;
  register float __y2 __asm__("fr9") = ty2;
  register float __y3 __asm__("fr10") = ty3; // in place already
  register float __y4 __asm__("fr11") = ty4;

  // This actually does a 4x4 matrix multiply to do the outer product.
  // It's this:
  //
  //  [ x1 x1 x1 x1 ] [ y1 0 0 0 ]     [ x1y1 x1y2 x1y3 x1y4 ]
  //  [ x2 x2 x2 x2 ] [ 0 y2 0 0 ]  =  [ x2y1 x2y2 x2y3 x2y4 ]
  //  [ x3 x3 x3 x3 ] [ 0 0 y3 0 ]     [ x3y1 x3y2 x3y3 x3y4 ]
  //  [ x4 x4 x4 x4 ] [ 0 0 0 y4 ]     [ x4y1 x4y2 x4y3 x4y4 ]
  //

  asm volatile (
    // zero out unoccupied front floats to make a double 0 in DR2
    "fldi0 FR2\n\t"
    "fmov FR2, FR3\n\t"
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs)
    // fmov 'x1' and 'x2' from FR4, FR5 into position (XF0,4,8,12, XF1,5,9,13)
    "fmov DR4, XD0\n\t"
    "fmov DR4, XD4\n\t"
    "fmov DR4, XD8\n\t"
    "fmov DR4, XD12\n\t"
    // fmov 'x3' and 'x4' from FR6, FR7 into position (XF2,6,10,14, XF3,7,11,15)
    "fmov DR6, XD2\n\t"
    "fmov DR6, XD6\n\t"
    "fmov DR6, XD10\n\t"
    "fmov DR6, XD14\n\t"
    // set up front floats (y1-y4)
    "fmov DR8, DR0\n\t"
    "fmov DR8, DR4\n\t"
    "fmov DR10, DR14\n\t"
    // finish zeroing out front floats
    "fmov DR2, DR6\n\t"
    "fmov DR2, DR8\n\t"
    "fmov DR2, DR12\n\t"
    "fschg\n\t" // switch back to single-move mode
    "fmov FR2, FR1\n\t"
    "fmov FR2, FR4\n\t"
    "fmov FR2, FR11\n\t"
    "fmov FR2, FR14\n\t"
    // finally, matrix multiply 4x4
    "ftrv XMTRX, FV0\n\t"
    "ftrv XMTRX, FV4\n\t"
    "ftrv XMTRX, FV8\n\t"
    "ftrv XMTRX, FV12\n\t"
    // Save output in XF regs
    "frchg\n"
  : // no outputs
  : "f" (__x1), "f" (__x2), "f" (__x3), "f" (__x4), "f" (__y1), "f" (__y2), "f" (__y3), "f" (__y4) // inputs
  : "fr0", "fr1", "fr2", "fr3", "fr12", "fr13", "fr14", "fr15" // clobbers, can't avoid it
  );
  // GCC will restore FR12-FR15 from the stack after this, so we really can't keep the output in the front bank.
}

// Matrix transform: matrix * vector = vector
//                   _    _       _    _
//  [ ----------- ] |  x1  |     |  z1  |
//  [ ---XMTRX--- ] |  x2  |  =  |  z2  |
//  [ ----------- ] |  x3  |     |  z3  |
//  [ ----------- ] |_ x4 _|     |_ z4 _|
//
// IMPORTANT USAGE INFORMATION (matrix transform):
//
// Return vector struct maps 1:1 to the above diagram:
//
//  typedef struct {
//   float z1;
//   float z2;
//   float z3;
//   float z4;
// } RETURN_VECTOR_STRUCT;
//
// Similarly to how the sine and cosine functions use fsca and return 2 floats,
// the matrix transform function actually returns 4 floats. The SH4 only multiplies
// 4x4 matrices with 4x1 vectors, which is why the output is like that.
//
// Multiply a matrix stored in the back bank (XMTRX) with an input vector
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT MATH_Matrix_Transform(float x1, float x2, float x3, float x4)
{
  // The floats comprising FV4 are the regs that are passed in.
  // Just need to make sure GCC doesn't modify anything, and these register vars do that job.

  // Temporary variables are necessary per GCC to avoid clobbering:
  // https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html#Local-Register-Variables

  float tx1 = x1;
  float tx2 = x2;
  float tx3 = x3;
  float tx4 = x4;

  // output vector FV0
  register float __z1 __asm__("fr0") = tx1;
  register float __z2 __asm__("fr1") = tx2;
  register float __z3 __asm__("fr2") = tx3;
  register float __z4 __asm__("fr3") = tx4;

  asm volatile ("ftrv XMTRX, FV0\n\t"
    // have to do this to obey SH4 calling convention--output returned in FV0
    : "+w" (__z1), "+f" (__z2), "+f" (__z3), "+f" (__z4) // outputs, "+" means r/w
    : // no inputs
    : // no clobbers
  );

  RETURN_VECTOR_STRUCT output = {__z1, __z2, __z3, __z4};
  return output;
}

// Matrix Transpose
//
// This does a matrix transpose on the matrix in XMTRX, which swaps rows with
// columns as follows (math notation is [XMTRX]^T):
//
//  [ a b c d ] T   [ a e i m ]
//  [ e f g h ]  =  [ b f j n ]
//  [ i j k l ]     [ c g k o ]
//  [ m n o p ]     [ d h l p ]
//
// PLEASE NOTE: It is faster to avoid the need for a transpose altogether by
// structuring matrices and vectors accordingly.
static inline __attribute__((always_inline)) void MATH_Matrix_Transpose(void)
{
  asm volatile ("frchg\n\t" // fmov for singles only works on front bank
    // FR0, FR5, FR10, and FR15 are already in place
    // swap FR1 and FR4
    "flds FR1, FPUL\n\t"
    "fmov FR4, FR1\n\t"
    "fsts FPUL, FR4\n\t"
    // swap FR2 and FR8
    "flds FR2, FPUL\n\t"
    "fmov FR8, FR2\n\t"
    "fsts FPUL, FR8\n\t"
    // swap FR3 and FR12
    "flds FR3, FPUL\n\t"
    "fmov FR12, FR3\n\t"
    "fsts FPUL, FR12\n\t"
    // swap FR6 and FR9
    "flds FR6, FPUL\n\t"
    "fmov FR9, FR6\n\t"
    "fsts FPUL, FR9\n\t"
    // swap FR7 and FR13
    "flds FR7, FPUL\n\t"
    "fmov FR13, FR7\n\t"
    "fsts FPUL, FR13\n\t"
    // swap FR11 and FR14
    "flds FR11, FPUL\n\t"
    "fmov FR14, FR11\n\t"
    "fsts FPUL, FR14\n\t"
    // restore XMTRX to back bank
    "frchg\n"
    : // no outputs
    : // no inputs
    : "fpul" // clobbers
  );
}

// Matrix product: matrix * matrix = matrix
//
// These use the whole dang floating point unit.
//
//  [ ----------- ] [ ----------- ]     [ ----------- ]
//  [ ---Back---- ] [ ---Front--- ]  =  [ ---XMTRX--- ]
//  [ ---Matrix-- ] [ ---Matrix-- ]     [ ----------- ]
//  [ --(XMTRX)-- ] [ ----------- ]     [ ----------- ]
//
// Multiply a matrix stored in the back bank with a matrix loaded from memory
// Output is stored in the back bank (XMTRX)
static inline __attribute__((always_inline)) void MATH_Matrix_Product(ALL_FLOATS_STRUCT * front_matrix)
{
  asm volatile ("pref @%[fmtrx]\n\t" // Prefetching should help a bit
    // gotta wait for 6 clocks (30ns) memory access time for pref to work
    "mov #32, r1\n\t"
    "add %[fmtrx], r1\n\t" // store offset by 32 in r1
    "pref @r1\n\t" // Get a head start prefetching the second half of the 64-byte data
    // NOPs are in the MT group, so they are executed in parallel...
    // all these nops should equal 2 cycles in this context...
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "fschg\n\t" // switch fmov to paired moves
    "fmov.d @%[fmtrx]+, DR0\n\t"
    "fmov.d @%[fmtrx]+, DR2\n\t"
    "fmov.d @%[fmtrx]+, DR4\n\t"
    "fmov.d @%[fmtrx]+, DR6\n\t"
    "fmov.d @%[fmtrx]+, DR8\n\t"
    "fmov.d @%[fmtrx]+, DR10\n\t"
    "fmov.d @%[fmtrx]+, DR12\n\t"
    "fmov.d @%[fmtrx], DR14\n\t"
    "fschg\n\t" // switch back to single moves
    // matrix multiply 4x4
    "ftrv XMTRX, FV0\n\t"
    "ftrv XMTRX, FV4\n\t"
    "ftrv XMTRX, FV8\n\t"
    "ftrv XMTRX, FV12\n\t"
    // Save output in XF regs
    "frchg\n"
    : [fmtrx] "+r" ((unsigned int)front_matrix) // outputs, "+" means r/w
    : // no inputs
    : "r1", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15" // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
  );
}

// Load two 4x4 matrices and multiply them, storing the output into the back bank (XMTRX)
//
// MATH_Load_Matrix_Product() is slightly faster than doing this:
//    MATH_Load_XMTRX(matrix1)
//    MATH_Matrix_Product(matrix2)
// as it saves having to do 2 extraneous 'fschg' instructions.
//
static inline __attribute__((always_inline)) void MATH_Load_Matrix_Product(ALL_FLOATS_STRUCT * matrix1, ALL_FLOATS_STRUCT * matrix2)
{
  asm volatile ("pref @%[bmtrx]\n\t" // Prefetching should help a bit
    // gotta wait for 6 clocks (30ns) memory access time for pref to work
    "mov #32, r0\n\t"
    "pref @%[fmtrx]\n\t" // prefetch fmtrx now while we wait
    "mov r0, r1\n\t" // This is parallel-issue
    "add %[bmtrx], r0\n\t" // store offset by 32 in r0
    "pref @r0\n\t" // Get a head start prefetching the second half of the 64-byte data
    "add %[fmtrx], r1\n\t" // store offset by 32 in r1
    "pref @r1\n\t" // likewise for other matrix
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs)
    // back matrix
    "fmov.d @%[bmtrx]+, XD0\n\t"
    "fmov.d @%[bmtrx]+, XD2\n\t"
    "fmov.d @%[bmtrx]+, XD4\n\t"
    "fmov.d @%[bmtrx]+, XD6\n\t"
    "fmov.d @%[bmtrx]+, XD8\n\t"
    "fmov.d @%[bmtrx]+, XD10\n\t"
    "fmov.d @%[bmtrx]+, XD12\n\t"
    "fmov.d @%[bmtrx], XD14\n\t"
    // front matrix
    "fmov.d @%[fmtrx]+, DR0\n\t"
    "fmov.d @%[fmtrx]+, DR2\n\t"
    "fmov.d @%[fmtrx]+, DR4\n\t"
    "fmov.d @%[fmtrx]+, DR6\n\t"
    "fmov.d @%[fmtrx]+, DR8\n\t"
    "fmov.d @%[fmtrx]+, DR10\n\t"
    "fmov.d @%[fmtrx]+, DR12\n\t"
    "fmov.d @%[fmtrx], DR14\n\t"
    "fschg\n\t" // switch back to single moves
    // matrix multiply 4x4
    "ftrv XMTRX, FV0\n\t"
    "ftrv XMTRX, FV4\n\t"
    "ftrv XMTRX, FV8\n\t"
    "ftrv XMTRX, FV12\n\t"
    // Save output in XF regs
    "frchg\n"
    : [bmtrx] "+&r" ((unsigned int)matrix1), [fmtrx] "+r" ((unsigned int)matrix2) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
    : // no inputs
    : "r0", "r1", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15" // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
  );
}

//------------------------------------------------------------------------------
// Matrix load and store operations
//------------------------------------------------------------------------------

// Load a matrix from memory into the back bank (XMTRX)
static inline __attribute__((always_inline)) void MATH_Load_XMTRX(ALL_FLOATS_STRUCT * back_matrix)
{
  asm volatile ("pref @%[bmtrx]\n\t" // Prefetching should help a bit
    // gotta wait for 6 clocks (30ns) memory access time for pref to work
    "mov #32, r1\n\t"
    "add %[bmtrx], r1\n\t" // store offset by 32 in r1
    "pref @r1\n\t" // Get a head start prefetching the second half of the 64-byte data
    // NOPs are in the MT group, so they are executed in parallel...
    // all these nops should equal 2 cycles in this context...
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs)
    "fmov.d @%[bmtrx]+, XD0\n\t"
    "fmov.d @%[bmtrx]+, XD2\n\t"
    "fmov.d @%[bmtrx]+, XD4\n\t"
    "fmov.d @%[bmtrx]+, XD6\n\t"
    "fmov.d @%[bmtrx]+, XD8\n\t"
    "fmov.d @%[bmtrx]+, XD10\n\t"
    "fmov.d @%[bmtrx]+, XD12\n\t"
    "fmov.d @%[bmtrx], XD14\n\t"
    "fschg\n" // switch back to single moves
    : [bmtrx] "+r" ((unsigned int)back_matrix) // outputs, "+" means r/w
    : // no inputs
    : "r1" // clobbers (GCC doesn't know about back bank, so writing to it isn't clobbered)
  );
}

// Store XMTRX to memory
static inline __attribute__((always_inline)) ALL_FLOATS_STRUCT * MATH_Store_XMTRX(ALL_FLOATS_STRUCT * destination)
{
  char * output = ((char*)destination) + sizeof(ALL_FLOATS_STRUCT) + 8; // ALL_FLOATS_STRUCT should be 64 bytes

  asm volatile ("pref @%[dest_base]\n\t"
    // gotta wait for 6 clocks (30ns) memory access time for pref to work
    "mov #32, r1\n\t"
    "add %[dest_base], r1\n\t" // store offset by 32 in r1
    "pref @r1\n\t" // Get a head start prefetching the second half of the 64-byte data
    // NOPs are in the MT group, so they are executed in parallel...
    // all these nops should equal 2 cycles in this context...
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "nop\n\t"
    "fschg\n\t" // switch fmov to paired moves (note: only paired moves can access XDn regs)
    "fmov.d XD0, @-%[out_mtrx]\n\t" // These do *(--output) = XDn
    "fmov.d XD2, @-%[out_mtrx]\n\t"
    "fmov.d XD4, @-%[out_mtrx]\n\t"
    "fmov.d XD6, @-%[out_mtrx]\n\t"
    "fmov.d XD8, @-%[out_mtrx]\n\t"
    "fmov.d XD10, @-%[out_mtrx]\n\t"
    "fmov.d XD12, @-%[out_mtrx]\n\t"
    "fmov.d XD14, @-%[out_mtrx]\n\t"
    "fschg\n" // switch back to single moves
    : [out_mtrx] "+&r" ((unsigned int)output) // outputs, "+" means r/w, "&" means it's written to before all inputs are consumed
    : [dest_base] "r" ((unsigned int)destination) // inputs
    : "r1", "memory" // clobbers
  );

  return destination;
}

// Returns FV0, 4, 8, or 12 from XMTRX
//
// Sorry, it has to be done 4 at a time like this due to calling convention
// limits; under optimal optimization conditions, we only get 4 float registers
// for return values; any more and they get pushed to memory.
//
// IMPORTANT USAGE INFORMATION (get XMTRX vector)
//
// XMTRX format, using the front bank's FVn notation:
//
//    FV0 FV4 FV8  FV12
//    --- --- ---  ----
//  [ xf0 xf4 xf8  xf12 ]
//  [ xf1 xf5 xf9  xf13 ]
//  [ xf2 xf6 xf10 xf14 ]
//  [ xf3 xf7 xf11 xf15 ]
//
// Return vector maps to XMTRX as below depending on the FVn value passed in:
//
//  typedef struct {
//   float z1; // will contain xf0, 4, 8 or 12
//   float z2; // will contain xf1, 5, 9, or 13
//   float z3; // will contain xf2, 6, 10, or 14
//   float z4; // will contain xf3, 7, 11, or 15
// } RETURN_VECTOR_STRUCT;
//
// Valid values of 'which' are 0, 4, 8, or 12, corresponding to FV0, FV4, FV8,
// or FV12, respectively. Other values will return 0 in all four return values.
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT MATH_Get_XMTRX_Vector(unsigned int which)
{
  register float __z1 __asm__("fr0");
  register float __z2 __asm__("fr1");
  register float __z3 __asm__("fr2");
  register float __z4 __asm__("fr3");

  // Note: only paired moves can access XDn regs
  asm volatile ("cmp/eq #0, %[select]\n\t" // if(which == 0), 1 -> T else 0 -> T
    "bt.s 0f\n\t" // do FV0
    " cmp/eq #4, %[select]\n\t" // if(which == 4), 1 -> T else 0 -> T
    "bt.s 4f\n\t" // do FV4
    " cmp/eq #8, %[select]\n\t" // if(which == 8), 1 -> T else 0 -> T
    "bt.s 8f\n\t" // do FV8
    " cmp/eq #12, %[select]\n\t" // if(which == 12), 1 -> T else 0 -> T
    "bf.s 1f\n" // exit if not even FV12 was true, otherwise do FV12
  "12:\n\t"
    " fschg\n\t" // paired moves for FV12 (and exit case)
    "fmov XD14, DR2\n\t"
    "fmov XD12, DR0\n\t"
    "bt.s 2f\n" // done
  "8:\n\t"
    " fschg\n\t" // paired moves for FV8, back to singles for FV12
    "fmov XD10, DR2\n\t"
    "fmov XD8, DR0\n\t"
    "bf.s 2f\n" // done
  "4:\n\t"
    " fschg\n\t" // paired moves for FV4, back to singles for FV8
    "fmov XD6, DR2\n\t"
    "fmov XD4, DR0\n\t"
    "bf.s 2f\n" // done
  "0:\n\t"
    " fschg\n\t" // paired moves for FV0, back to singles for FV4
    "fmov XD2, DR2\n\t"
    "fmov XD0, DR0\n\t"
    "bf.s 2f\n" // done
  "1:\n\t"
    " fschg\n\t" // back to singles for FV0 and exit case
    "fldi0 FR0\n\t" // FR0-3 get zeroed out, then
    "fmov FR0, FR1\n\t"
    "fmov FR0, FR2\n\t"
    "fmov FR0, FR3\n"
  "2:\n"
    : "=w" (__z1), "=f" (__z2), "=f" (__z3), "=f" (__z4) // outputs
    : [select] "z" (which) // inputs
    : "t" // clobbers
  );

  RETURN_VECTOR_STRUCT output = {__z1, __z2, __z3, __z4};
  return output;
}

// Returns a 2x2 matrix from a quadrant of XMTRX
//
// Sorry, it has to be done 4 at a time like this due to calling convention
// limits; under optimal optimization conditions, we only get 4 float registers
// for return values; any more and they get pushed to memory.
//
// IMPORTANT USAGE INFORMATION (get XMTRX 2x2)
//
// Each 2x2 quadrant is of the form:
//
//  [ a b ]
//  [ c d ]
//
// Return vector maps to the 2x2 matrix as below:
//
//  typedef struct {
//   float z1; // a
//   float z2; // c
//   float z3; // b
//   float z4; // d
// } RETURN_VECTOR_STRUCT;
//
//  (So the function does a 2x2 transpose in storing the values relative to the
//  order stored in XMTRX.)
//
// Valid values of 'which' are 1, 2, 3, or 4, corresponding to the following
// quadrants of XMTRX:
//
//       1             2
//  [ xf0 xf4 ] | [ xf8 xf12 ]
//  [ xf1 xf5 ] | [ xf9 xf13 ]
//  --   3   -- |  --  4  --
//  [ xf2 xf6 ] | [ xf10 xf14 ]
//  [ xf3 xf7 ] | [ xf11 xf15 ]
//
// Other input values will return 0 in all four return floats.
static inline __attribute__((always_inline)) RETURN_VECTOR_STRUCT MATH_Get_XMTRX_2x2(unsigned int which)
{
  register float __z1 __asm__("fr0");
  register float __z2 __asm__("fr1");
  register float __z3 __asm__("fr2");
  register float __z4 __asm__("fr3");

  // Note: only paired moves can access XDn regs
  asm volatile ("cmp/eq #1, %[select]\n\t" // if(which == 1), 1 -> T else 0 -> T
    "bt.s 1f\n\t" // do quadrant 1
    " cmp/eq #2, %[select]\n\t" // if(which == 2), 1 -> T else 0 -> T
    "bt.s 2f\n\t" // do quadrant 2
    " cmp/eq #3, %[select]\n\t" // if(which == 3), 1 -> T else 0 -> T
    "bt.s 3f\n\t" // do quadrant 3
    " cmp/eq #4, %[select]\n\t" // if(which == 4), 1 -> T else 0 -> T
    "bf.s 0f\n" // exit if nothing was true, otherwise do quadrant 4
  "4:\n\t"
    " fschg\n\t" // paired moves for quadrant 4 (and exit case)
    "fmov XD14, DR2\n\t"
    "fmov XD10, DR0\n\t"
    "bt.s 5f\n" // done
  "3:\n\t"
    " fschg\n\t" // paired moves for quadrant 3, back to singles for 4
    "fmov XD6, DR2\n\t"
    "fmov XD2, DR0\n\t"
    "bf.s 5f\n" // done
  "2:\n\t"
    " fschg\n\t" // paired moves for quadrant 2, back to singles for 3
    "fmov XD12, DR2\n\t"
    "fmov XD8, DR0\n\t"
    "bf.s 5f\n" // done
  "1:\n\t"
    " fschg\n\t" // paired moves for quadrant 1, back to singles for 2
    "fmov XD4, DR2\n\t"
    "fmov XD0, DR0\n\t"
    "bf.s 5f\n" // done
  "0:\n\t"
    " fschg\n\t" // back to singles for quadrant 1 and exit case
    "fldi0 FR0\n\t" // FR0-3 get zeroed out, then
    "fmov FR0, FR1\n\t"
    "fmov FR0, FR2\n\t"
    "fmov FR0, FR3\n"
  "5:\n"
    : "=w" (__z1), "=f" (__z2), "=f" (__z3), "=f" (__z4) // outputs
    : [select] "z" (which) // inputs
    : "t" // clobbers
  );

  RETURN_VECTOR_STRUCT output = {__z1, __z2, __z3, __z4};
  return output;
}

// It is not possible to return an entire 4x4 matrix in registers, as the only
// registers allowed for return values are R0-R3 and FR0-FR3. All others are
// marked caller save, which means they could be restored from stack and clobber
// anything returned in them.
//
// In general, writing the entire required math routine in one asm function is
// the best way to go for performance reasons anyways, and in that situation one
// can just throw calling convention to the wind until returning back to C.

#endif /* __SH4_MATH_H_ */