/*
 * Copyright 2017-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_clock.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.clock"
#endif
/* LPCG register Bits Mask. LPCG register generally comes in four bit groups, a STOP, Reserved, [P|B]CG(SWEN), HWEN
 * nibble. The HWEN bits are optional, but the allocations of the nibble always be on four bit boundaries. */
#define LPCG_CLK_HWEN_MASK (0x11111111U)
#define LPCG_CLK_SWEN_MASK (0x22222222U)
#define LPCG_CLK_STOP_MASK (0x88888888U)

/*******************************************************************************
 * Variables
 ******************************************************************************/
static sc_ipc_t ipcHandle;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Initialize Clock module.
 *
 * param ipc  IPC handle for communication with SCU, see \ref sc_ipc_t.
 */
void CLOCK_Init(sc_ipc_t ipc)
{
    ipcHandle = ipc;
}

/*!
 * brief Deinitialize Clock module.
 */
void CLOCK_Deinit(void)
{
}

/*!
 * brief Enable the clock for specific IP, with gate setting.
 *
 * param name  Which clock to enable, see \ref clock_ip_name_t.
 * param gate  0: clock always on, 1: HW auto clock gating.
 * return true if success, false if failure.
 */
bool CLOCK_EnableClockExt(clock_ip_name_t name, uint32_t gate)
{
	return CLOCK_EnableClockExtMapped((uint32_t *)LPCG_TUPLE_REG_BASE(name), name, gate);
}


/*!
 * brief Enable the clock for specific IP, with gate setting (mapped version).
 *
 * param lpcgBase Virtual/physical base address of the LPCG region associated with IP.
 * param name  Which clock to enable, see \ref clock_ip_name_t.
 * param gate  0: clock always on, 1: HW auto clock gating.
 * return true if success, false if failure.
 */
bool CLOCK_EnableClockExtMapped(uint32_t *lpcgBase, clock_ip_name_t name, uint32_t gate)
{
    sc_err_t err;

    err = sc_pm_clock_enable(ipcHandle, LPCG_TUPLE_RSRC(name), SC_PM_CLK_PER, true, (bool)gate);

    /* Enable the Clock Gate control in LPCG */
    CLOCK_ConfigLPCGMapped(lpcgBase, name, true, (bool)gate);

    if (err != SC_ERR_NONE)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/*!
 * brief Disable the clock for specific IP.
 *
 * param name  Which clock to disable, see \ref clock_ip_name_t.
 * return true for success, false for failure.
 */
bool CLOCK_DisableClock(clock_ip_name_t name)
{
	return CLOCK_DisableClockMapped((uint32_t *)LPCG_TUPLE_REG_BASE(name), name);
}

/*!
 * brief Disable the clock for specific IP (mapped version).
 *
 * param lpcgBase Virtual/physical base address of the LPCG region associated with IP.
 * param name  Which clock to disable, see \ref clock_ip_name_t.
 * return true for success, false for failure.
 */
bool CLOCK_DisableClockMapped(uint32_t *lpcgBase, clock_ip_name_t name)
{
    sc_err_t err;

    /* Disable the Clock Gate control in LPCG */
    CLOCK_ConfigLPCGMapped(lpcgBase, name, false, false);

    err = sc_pm_clock_enable(ipcHandle, LPCG_TUPLE_RSRC(name), SC_PM_CLK_PER, false, false);

    if (err != SC_ERR_NONE)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/*!
 * brief Set the clock frequency for specific IP module.
 *
 * This function sets the IP module clock frequency.
 *
 * param name Which peripheral to check, see \ref clock_ip_name_t.
 * param freq Target clock frequency value in hertz.
 * return the Real clock frequency value in hertz, or 0 if failed
 */
uint32_t CLOCK_SetIpFreq(clock_ip_name_t name, uint32_t freq)
{
    uint32_t target = freq;
    sc_err_t err;

    err = sc_pm_set_clock_rate(ipcHandle, LPCG_TUPLE_RSRC(name), SC_PM_CLK_PER, &target);
    if (err != SC_ERR_NONE)
    {
        return 0;
    }
    else
    {
        return target;
    }
}

/*!
 * brief Get the clock frequency for a specific IP module.
 *
 * This function gets the IP module clock frequency.
 *
 * param name Which peripheral to get, see \ref clock_ip_name_t.
 * return Clock frequency value in hertz, or 0 if failed
 */
uint32_t CLOCK_GetIpFreq(clock_ip_name_t name)
{
    uint32_t freq;
    sc_err_t err;

    err = sc_pm_get_clock_rate(ipcHandle, LPCG_TUPLE_RSRC(name), SC_PM_CLK_PER, &freq);
    if (err != SC_ERR_NONE)
    {
        return 0;
    }
    else
    {
        return freq;
    }
}

/*!
 * brief Get the core clock or system clock frequency.
 *
 * return Clock frequency in Hz.
 */
uint32_t CLOCK_GetCoreSysClkFreq(void)
{
    uint32_t freq;
    sc_err_t err;
#if defined(MIMX8QM_CM4_CORE0)
    err = sc_pm_get_clock_rate(ipcHandle, SC_R_M4_0_PID0, SC_PM_CLK_PER, &freq);
#elif defined(MIMX8QM_CM4_CORE1)
    err = sc_pm_get_clock_rate(ipcHandle, SC_R_M4_1_PID0, SC_PM_CLK_PER, &freq);
#elif defined(CONFIG_SOC_MIMX8QM_A53)
    err = sc_pm_get_clock_rate(ipcHandle, SC_R_A53, SC_PM_CLK_PER, &freq);
#else
#error "Clock driver not support the core!"
#endif
    if (err != SC_ERR_NONE)
    {
        freq = 0U;
    }

    return freq;
}

/*!
 * brief Gets the clock frequency for a specific clock name.
 *
 * This function checks the current clock configurations and then calculates
 * the clock frequency for a specific clock name defined in clock_name_t.
 *
 * param clockName Clock names defined in clock_name_t
 * return Clock frequency value in hertz
 */
uint32_t CLOCK_GetFreq(clock_name_t name)
{
    uint32_t freq;

    switch (name)
    {
        case kCLOCK_CONECTIVITY_AhbClk:
            freq = 167000000U; /* The CONNECTIVITY SS AHB clock is fixed to 166MHZ */
            break;
        case kCLOCK_CoreSysClk:
            freq = CLOCK_GetCoreSysClkFreq();
            break;
        default:
            freq = 0U;
            break;
    }

    return freq;
}

/*!
 * brief Set LPCG gate for specific LPCG.
 *
 * param regBase LPCG register base address.
 * param swGate Software clock gating. 0: clock is gated;  1: clock is enabled
 * param hwGate Hardware auto gating. 0: disable the HW clock gate control;  1: HW clock gating is enabled
 * param bitsMask The available bits in LPCG register. Each bit indicate the corresponding bit is available or not.
 */
void CLOCK_SetLpcgGate(volatile uint32_t *regBase, bool swGate, bool hwGate, uint32_t bitsMask)
{
    if (regBase != NULL)
    {
        if (swGate)
        {
            *regBase |= bitsMask & LPCG_CLK_SWEN_MASK;
        }
        else
        {
            *regBase &= ~(bitsMask & LPCG_CLK_SWEN_MASK);
        }

        if (hwGate)
        {
            *regBase |= bitsMask & LPCG_CLK_HWEN_MASK;
        }
        else
        {
            *regBase &= ~(bitsMask & LPCG_CLK_HWEN_MASK);
        }
    }
}

/*!
 * brief Config the LPCG cell for specific IP.
 *
 * param name  Which clock to enable, see \ref clock_ip_name_t.
 * param swGate Software clock gating. 0: clock is gated;  1: clock is enabled
 * param swGate Hardware auto gating. 0: disable the HW clock gate control;  1: HW clock gating is enabled
 */
void CLOCK_ConfigLPCG(clock_ip_name_t name, bool swGate, bool hwGate)
{
    CLOCK_ConfigLPCGMapped((uint32_t *)LPCG_TUPLE_REG_BASE(name), name, swGate, hwGate);
}

/*!
 * brief Config the LPCG cell for specific IP (mapped version).
 *
 * param lpcgBase Virtual/physical base address of the LPCG region associated with IP.
 * param name  Which clock to enable, see \ref clock_ip_name_t.
 * param swGate Software clock gating. 0: clock is gated;  1: clock is enabled
 * param swGate Hardware auto gating. 0: disable the HW clock gate control;  1: HW clock gating is enabled
 */
void CLOCK_ConfigLPCGMapped(uint32_t *lpcgBase, clock_ip_name_t name, bool swGate, bool hwGate)
{
    volatile uint32_t *regBase;

    regBase = lpcgBase;

    /* Return if LPCG Cell is not available. */
    if (regBase == NULL)
    {
        return;
    }

    /* Config the LPCG. LPCG Cells have different configurations per each clock target. */
    switch (name)
    {
        /* LPCG cell avalialbe bits field mask 0xBBAAAB, 0xBBAAAB (2 32-bits LPCG registers). */
        case kCLOCK_CONNECTIVITY_Enet0:
        case kCLOCK_CONNECTIVITY_Enet1:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xBBAAABU);
            CLOCK_SetLpcgGate(&regBase[1U], swGate, hwGate, 0xAU);
            break;

        /* LPCG cell avalialbe bits field mask 0xA000B.*/
        case kCLOCK_AUDIO_Esai0:
        case kCLOCK_AUDIO_Esai1:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xA000BU);
            break;

        /* LPCG cell avalialbe bits field mask 0xAA000B.*/
        case kCLOCK_AUDIO_Gpt0:
        case kCLOCK_AUDIO_Gpt1:
        case kCLOCK_AUDIO_Gpt2:
        case kCLOCK_AUDIO_Gpt3:
        case kCLOCK_AUDIO_Gpt4:
        case kCLOCK_AUDIO_Gpt5:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xAA000BU);
            break;

        /* LPCG cell avalialbe bits field mask 0xAAB0AAB.*/
        case kCLOCK_LSIO_Gpt0:
        case kCLOCK_LSIO_Gpt1:
        case kCLOCK_LSIO_Gpt2:
        case kCLOCK_LSIO_Gpt3:
        case kCLOCK_LSIO_Gpt4:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xAAB0AABU);
            break;

        /* LPCG cell avalialbe bits field mask 0xB0000.*/
        case kCLOCK_HSIO_Gpio:
        case kCLOCK_LSIO_Gpio0:
        case kCLOCK_LSIO_Gpio1:
        case kCLOCK_LSIO_Gpio2:
        case kCLOCK_LSIO_Gpio3:
        case kCLOCK_LSIO_Gpio4:
        case kCLOCK_LSIO_Gpio5:
        case kCLOCK_LSIO_Gpio6:
        case kCLOCK_LSIO_Gpio7:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xB0000U);
            break;
        /* LPCG cell avalialbe bits field mask 0xB000A. */
        case kCLOCK_DMA_Lpadc0:
        case kCLOCK_DMA_Lpadc1:
        case kCLOCK_LSIO_Mu5A:
        case kCLOCK_LSIO_Mu6A:
        case kCLOCK_LSIO_Mu7A:
        case kCLOCK_LSIO_Mu8A:
        case kCLOCK_LSIO_Mu9A:
        case kCLOCK_LSIO_Mu10A:
        case kCLOCK_LSIO_Mu11A:
        case kCLOCK_LSIO_Mu12A:
        case kCLOCK_LSIO_Mu13A:
        case kCLOCK_LSIO_Mu5B:
        case kCLOCK_LSIO_Mu6B:
        case kCLOCK_LSIO_Mu7B:
        case kCLOCK_LSIO_Mu8B:
        case kCLOCK_LSIO_Mu9B:
        case kCLOCK_LSIO_Mu10B:
        case kCLOCK_LSIO_Mu11B:
        case kCLOCK_LSIO_Mu12B:
        case kCLOCK_LSIO_Mu13B:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xB000AU);
            break;

        /* LPCG cell avalialbe bits field mask 0xB000B. */
        case kCLOCK_DMA_Lpspi0:
        case kCLOCK_DMA_Lpspi1:
        case kCLOCK_DMA_Lpspi2:
        case kCLOCK_DMA_Lpspi3:
        case kCLOCK_DMA_Lpuart0:
        case kCLOCK_DMA_Lpuart1:
        case kCLOCK_DMA_Lpuart2:
        case kCLOCK_DMA_Lpuart3:
        case kCLOCK_DMA_Lpuart4:
        case kCLOCK_DMA_Lpi2c0:
        case kCLOCK_DMA_Lpi2c1:
        case kCLOCK_DMA_Lpi2c2:
        case kCLOCK_DMA_Lpi2c3:
        case kCLOCK_DMA_Lpi2c4:
        case kCLOCK_DMA_Ftm0:
        case kCLOCK_DMA_Ftm1:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xB000BU);
            break;

        /* LPCG cell avalialbe bits field mask 0xA0B0A.*/
        case kCLOCK_AUDIO_Sai0:
        case kCLOCK_AUDIO_Sai1:
        case kCLOCK_AUDIO_Sai2:
        case kCLOCK_AUDIO_Sai3:
        case kCLOCK_AUDIO_Sai4:
        case kCLOCK_AUDIO_Sai5:
        case kCLOCK_AUDIO_Sai6:
        case kCLOCK_AUDIO_Sai7:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xA0B0AU);
            break;

        /* LPCG cell avalialbe bits field mask 0xBB000B.*/
        case kCLOCK_DMA_Can0:
        case kCLOCK_DMA_Can1:
        case kCLOCK_DMA_Can2:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xBB000B);
            break;

        /* LPCG cell avalialbe bits field mask 0xBAA000A.*/
        case kCLOCK_LSIO_Flexspi0:
        case kCLOCK_LSIO_Flexspi1:
            CLOCK_SetLpcgGate(regBase, swGate, hwGate, 0xBAA000A);
            break;

        /* LPCG cell is not avaliable or is not supported by this function. */
        default:
            /* Add comments to avoid the violation of MISRA C-2012 rule 16.4. */
            break;
    }
}
