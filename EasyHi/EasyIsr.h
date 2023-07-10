#ifndef __EASYISR_H__
#define __EASYISR_H__

#include "hi_types_base.h"
#include "hi3861.h"
#include "hi_isr.h"
#include "EasyError.h"



/**
 * @brief Easy Isr Init Structures
 *
 */
typedef struct EasyIsr_Init
{
    hi_u8 DEVICE;            /* the device you would like to enable the Isr, see "hi3861.h" */
    hi_u8 PRIORITY;          /* the priority of the Isr, from 1 to 7 */
    irq_routine CALLBACKFUNC; /* the function for calling back */
    hi_u32 CALLBACKPARA;      /* the param of the callback function */
} Typedef_EasyIsrInit;

EasyHi_Error EasyIsr_Init(Typedef_EasyIsrInit *EasyIsrStructure);
EasyHi_Error EasyIsr_DeInit(Typedef_EasyIsrInit *EasyIsrStructure);
EasyHi_Error EasyIsr_Start(Typedef_EasyIsrInit *EasyIsrStructure);
void EasyIsr_Stop(Typedef_EasyIsrInit *EasyIsrStructure);
void EasyIsr_DisableAll(void);
void EasyIsr_EnableAll(void);

#endif