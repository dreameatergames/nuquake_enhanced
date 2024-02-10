#pragma once
#ifndef CYGPROFILE_H_
#define CYGPROFILE_H_

/* Based on the idea from Erich Styger */
/* profiled instrument guided profiling for gldc on hardware */

#define NO_INSTRUMENT inline __attribute__((no_instrument_function))
#define INLINE_DEBUG NO_INSTRUMENT __attribute__((always_inline))
#define INLINE_ALWAYS static NO_INSTRUMENT __attribute__((always_inline))

extern char _etext;
#define BASE_ADDRESS 0x8c010000

#define CYG_FUNC_TRACE_ENABLED (1)
/*!< 1: Trace enabled, 0: trace disabled */

/*!
 * \brief Print the call trace to the terminal.
 */
void CYG_PrintCallTrace(void);

/*!
 * \brief Driver Initialization.
 */
void CYG_Init(void);

/*!
 * \brief Driver De-Initialization.
 */
void CYG_Deinit(void);

#endif /* CYGPROFILE_H_ */