#include "hi_early_debug.h"
#include "EasyIsr.h"
#include "EasyTimer.h"

// init a variable to restore the status of Isr
static hi_u32 EasyIsr_Status = 0;

/**
 * @brief a weak callback function, you need to redefine it in your project
 *
 */
__attribute__((weak)) void EasyIsr_AllCallbackFunc(Typedef_EasyIsrInit *EasyIsr)
{
    // remember to reset the flag of Isr
}

/**
 * @brief initialize Isr with your config
 *
 * @param EasyIsrStructure the config of Isr
 * @return EasyHi_Error: Success: 0 ; more details please go to EasyError.h
 */
EasyHi_Error EasyIsr_Init(Typedef_EasyIsrInit *EasyIsrStructure)
{
    hi_u32 Ack = 0;

    if (EasyIsrStructure->DEVICE < 26 || EasyIsrStructure->DEVICE > 61)
        return ISR_DEVICEERROR;

    if (EasyIsrStructure->PRIORITY < 1 || EasyIsrStructure->PRIORITY > 7)
        return ISR_PRIORITYERROR;

    // disable the isr
    hi_irq_disable(EasyIsrStructure->DEVICE);

    // init
    if (EasyIsrStructure->CALLBACKFUNC == NULL)
    {
        Ack = hi_irq_request((hi_u32)EasyIsrStructure->DEVICE,
                             (hi_u32)EasyIsrStructure->PRIORITY,
                             (irq_routine)EasyIsr_AllCallbackFunc,
                             &EasyIsrStructure);
    }
    else
    {
        Ack = hi_irq_request((hi_u32)EasyIsrStructure->DEVICE,
                             (hi_u32)EasyIsrStructure->PRIORITY,
                             EasyIsrStructure->CALLBACKFUNC,
                             EasyIsrStructure->CALLBACKPARA);
    }

    if (Ack == HI_ERR_SUCCESS)
        return EasyHi_SUCCESS;
    else
        return Ack;
}

/**
 * @brief clear and deinitialize the Isr
 *
 * @param EasyIsrStructure the Isr you would like to deinit
 * @return EasyHi_Error: Success: 0 ; more details please go to EasyError.h
 */
EasyHi_Error EasyIsr_DeInit(Typedef_EasyIsrInit *EasyIsrStructure)
{
    hi_u32 Ack = 0;

    Ack = hi_irq_free((hi_u32)EasyIsrStructure->DEVICE);

    if (Ack == HI_ERR_SUCCESS)
        return EasyHi_SUCCESS;
    else
        return Ack;
}

/**
 * @brief Start an Isr
 *
 * @param EasyIsrStructure the Isr you would like to start
 * @return EasyHi_Error: Success: 0 ; more details please go to EasyError.h
 */
EasyHi_Error EasyIsr_Start(Typedef_EasyIsrInit *EasyIsrStructure)
{
    hi_u32 Ack = 0;

    Ack = hi_irq_enable((hi_u32)EasyIsrStructure->DEVICE);

    if (Ack == HI_ERR_SUCCESS)
        return EasyHi_SUCCESS;
    else
        return Ack;
}

/**
 * @brief Stop an Isr
 *
 * @param EasyIsrStructure the Isr you would like to stop
 */
void EasyIsr_Stop(Typedef_EasyIsrInit *EasyIsrStructure)
{
    hi_irq_disable((hi_u32)EasyIsrStructure->DEVICE);
}

/**
 * @brief disable all the Isr
 *
 */
void EasyIsr_DisableAll(void)
{
    EasyIsr_Status = hi_int_lock();
}

/**
 * @brief enable all the Isr
 *
 */
void EasyIsr_EnableAll(void)
{
    hi_int_restore(EasyIsr_Status);
}