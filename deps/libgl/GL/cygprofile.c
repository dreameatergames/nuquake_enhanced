/* Based on the idea from Erich Styger */
/* profiled instrument guided profiling for gldc on hardware */

#include "cygprofile.h"
#include <kos.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "perfctr.h"
#include "private.h"

#if CYG_FUNC_TRACE_ENABLED

#define _strcat(x, y, z) strncat(x, z, y)

#ifndef __PE_Error_H
#define __PE_Error_H

#define ERR_OK 0       /* OK */
#define ERR_SPEED 1    /* This device does not work in the active speed mode. */
#define ERR_RANGE 2    /* Parameter out of range. */
#define ERR_VALUE 3    /* Parameter of incorrect value. */
#define ERR_OVERFLOW 4 /* Timer overflow. */
#define ERR_MATH 5     /* Overflow during evaluation. */
#define ERR_ENABLED 6  /* Device is enabled. */
#define ERR_DISABLED 7 /* Device is disabled. */
#define ERR_BUSY 8     /* Device is busy. */
#define ERR_NOTAVAIL 9 /* Requested value or method not available. */
#define ERR_RXEMPTY 10 /* No data in receiver. */
#define ERR_TXFULL 11  /* Transmitter is full. */
#define ERR_BUSOFF 12  /* Bus not available. */
#define ERR_OVERRUN 13 /* Overrun error is detected. */
#define ERR_FRAMING 14 /* Framing error is detected. */
#define ERR_PARITY 15  /* Parity error is detected. */
#define ERR_NOISE 16   /* Noise error is detected. */
#define ERR_IDLE 17    /* Idle error is detectes. */
#define ERR_FAULT 18   /* Fault error is detected. */
#define ERR_BREAK 19   /* Break char is received during communication. */
#define ERR_CRC 20     /* CRC error is detected. */
#define ERR_ARBITR 21  /* A node losts arbitration. This error occurs if two nodes start transmission at the same time. */
#define ERR_PROTECT 22 /* Protection error is detected. */

#endif /* __PE_Error_H */

#define CYG_RNG_BUF_NOF_ELEMS (8096 * 4)
/*!< Number of elements in the ring buffer which is used to record function calls */
#define CYG_THUMB_MASK 0xFFFFFFFF
/*!< mask out LSB (thumb) bit */

/* Hashing function for two uint32_ts */
#define HASH_PAIR(x, y) (((x)*0x1f1f1f1f) ^ (y))

static bool CYG_Enabled = false; /*!< flag which enables/disables tracing */

/*!
 * Element in ring buffer to store the trace information.
 */
typedef struct
{
  //bool isEnter;    /*!< TRUE for __cyg_profile_func_enter(), FALSE for __cyg_profile_func_exit() */
  void *this_fn;    /*!< address (with thumb bit) of the (caller) function */
  void *call_site;  /*!< return address to the function which called this_fn */
  uint32_t counter; /* also contains isEnter as highest bit */
} CYG_RNG_ElementType;

typedef uint32_t CYG_RNG_BufSizeType; /*!< index type for ring buffer */

static CYG_RNG_ElementType CYG_RNG_buffer[CYG_RNG_BUF_NOF_ELEMS]; /*!< ring buffer */
//static CYG_RNG_BufSizeType CYG_RNG_inIdx;                         /*!< input index */
static CYG_RNG_BufSizeType CYG_RNG_outIdx; /*!< output index */
static CYG_RNG_BufSizeType CYG_RNG_inSize; /*!< size/number of elements in buffer */

/*!
 * \brief Stores a trace element into the ring buffer.
 * \param elem Trace element to put into the buffer.
 * \return Error code, ERR_OK if everything is ok.
 */
__attribute__((no_instrument_function)) static uint8_t CYG_RNG_Put(CYG_RNG_ElementType *elem) {
  uint8_t res = ERR_OK;

#if 0
    if (CYG_RNG_inSize == CYG_RNG_BUF_NOF_ELEMS)
    {
        res = ERR_TXFULL;
        CYG_RNG_inSize--;
        CYG_PrintCallTrace();
        //CYG_RNG_inIdx = 0;
        CYG_RNG_outIdx = 0;
        CYG_RNG_inSize = 0;
        return CYG_RNG_Put(elem);
    }
    else
    {
        //CYG_RNG_buffer[CYG_RNG_inIdx] = *elem;
        
        /*
        CYG_RNG_inIdx++;
        if (CYG_RNG_inIdx == CYG_RNG_BUF_NOF_ELEMS)
        {
            CYG_RNG_inIdx = 0;
        }
        */
        CYG_RNG_inSize++;
    }
#endif
  CYG_RNG_ElementType *possible = &CYG_RNG_buffer[HASH_PAIR((uint32_t)elem->call_site, (uint32_t)elem->this_fn) % CYG_RNG_BUF_NOF_ELEMS];
  if (possible->counter /*& 0x0FFFFFFF*/ == 0) {
    *possible = *elem;
  } else {
    possible->counter++;
  }
  return res;
}

/*!
 * \brief Gets a trace element from the ring buffer.
 * \param elem Pointer where to store the trace element.
 * \return Error code, ERR_OK if everything is ok.
 */
__attribute__((no_instrument_function)) static uint8_t CYG_RNG_Get(CYG_RNG_ElementType *elemP) {
  uint8_t res = ERR_OK;

  if (CYG_RNG_inSize == 0) {
    res = ERR_RXEMPTY;
  } else {
    *elemP = CYG_RNG_buffer[CYG_RNG_outIdx];
    CYG_RNG_inSize--;
    CYG_RNG_outIdx++;
    if (CYG_RNG_outIdx == CYG_RNG_BUF_NOF_ELEMS) {
      CYG_RNG_outIdx = 0;
    }
  }
  return res;
}

static uint32_t currentTime[2];
static uint32_t lastTime;

/*!
 * \brief Stores a trace element into the ring buffer.
 * \param this_fn Address of the caller function.
 * \param call_site Return address to the function which called this_fn
 * \return Error code, ERR_OK if everything is ok.
 */
__attribute__((no_instrument_function)) static void CYG_Store(void *this_fn, void *call_site) {
  CYG_RNG_ElementType elem;
  lastTime = currentTime[0];
  PMCR_Read(1, (unsigned int *)currentTime);
  //elem.isEnter = isEnter;
  elem.call_site = call_site;
  elem.this_fn = this_fn;
  elem.counter = 1;  //currentTime[0] - lastTime;
  CYG_RNG_Put(&elem);
}

/*!
 * \brief Function which is called upon function enter. The function call is inserted by the compiler.
 * \param this_fn Address of the caller function.
 * \param call_site Return address to the function which called this_fn
 */
__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (CYG_Enabled) {
    CYG_Store(call_site, this_fn);
  }
}

/*!
 * \brief Function which is called upon function exit. The function call is inserted by the compiler.
 * \param this_fn Address of the caller function.
 * \param call_site Return address to the function which called this_fn
 */
__attribute__((no_instrument_function)) void __cyg_profile_func_exit(__attribute__((unused)) void *this_fn, __attribute__((unused)) void *call_site) {
}

/*!
 * \brief Dumps the trace to the console.
 */
__attribute__((no_instrument_function)) void CYG_PrintCallTrace(void) {
  CYG_RNG_BufSizeType i;
  char buf[40];
  CYG_RNG_ElementType elem;
  uint8_t res;

  CYG_Enabled = false;
  printf("0x%08x\n", ((unsigned int)&_etext) - BASE_ADDRESS);
  //printf("Function Trace:\r\n");
  CYG_RNG_outIdx = 0;
  for (i = 0; i < CYG_RNG_BUF_NOF_ELEMS; i++) {
    buf[0] = '\0';
    res = CYG_RNG_Get(&elem);
    if (res == ERR_OK && elem.call_site != NULL) {
      snprintf(buf, sizeof(buf), "{ 0x%" PRIXPTR " 0x%" PRIXPTR " %u\r\n", (uintptr_t)(elem.this_fn) & CYG_THUMB_MASK, (uintptr_t)(elem.call_site) & CYG_THUMB_MASK, (unsigned int)elem.counter);

      printf(buf);
    } else {
      //printf("ERROR getting element!\r\n");
    }
  }
  //printf("Function Trace: done!\r\n");
}

__attribute__((no_instrument_function)) void CYG_Init(void) {
  if (CYG_Enabled) {
    return;
  }
  CYG_RNG_inSize = CYG_RNG_BUF_NOF_ELEMS;
  CYG_RNG_outIdx = 0;
  CYG_Enabled = true;
  currentTime[0] = currentTime[1] = 0;
  lastTime = 0;
  memset(CYG_RNG_buffer, 0, sizeof(CYG_RNG_buffer));
  PMCR_Init(1, PMCR_ELAPSED_TIME_MODE, PMCR_COUNT_CPU_CYCLES);
}

__attribute__((no_instrument_function)) void CYG_Deinit(void) {
  CYG_RNG_inSize = CYG_RNG_BUF_NOF_ELEMS;
  CYG_RNG_outIdx = 0;
  CYG_Enabled = false;
  memset(CYG_RNG_buffer, 0, sizeof(CYG_RNG_buffer));
}
#else

void CYG_PrintCallTrace(void){}
void CYG_Init(void){}
void CYG_Deinit(void){}

#endif
