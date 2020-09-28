/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer *
* Copyright (C) 2012 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name	   : r_s2_peripheral_if.h
* Version      : 1.0
* Description  : This module solves all the world's problems
******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 20.09.2012 1.00     First Release
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "r_stdint.h"
#include "r_adpcm.h"
#include "r_s2_peripheral_if.h"
#include "platform.h"


/******************************************************************************
Private global variables and functions
******************************************************************************/
/* MTUm.TRGy register set value (When RX64M PCLKA = 120MHz.) */
const uint16_t mtu_tgrx_set_table[TIMER_FREQ_LIST_MAX] =
{
	14999,	/*      8kHz */
	10883,	/* 11.025kHz */
	7499,	/*     16kHz */
	5441,	/* 22.050kHz */
	3749,	/*     32kHz */
	2720	/* 44.100kHz */
};



/**************************************************/
/* Decode operation                               */
/**************************************************/
/* Interval timer setting */
void decode_interval_timer_init(uint8_t sampling_rate)
{
	SYSTEM.PRCR.WORD = 0xA500;
	SYSTEM.PRCR.WORD = 0xA502;
	MSTP_MTU = 0;				/* MTU unit (MTU0 to MTU5) The module stop state is canceled */
	SYSTEM.PRCR.WORD = 0xA500;

	/* MTU1 timer control register */
	MTU1.TCR.BYTE = 0x20;		/*  b7:b5  CCLR2:CCLR0 - counter clear bit - TCNT counter cleared by TGRB compare match/input capture.
									b4:b3  CKE1:CKE0   - clock select bit  - Counted at falling edge
									b2:b0  TPSC2:TPSC0 - mode select bit   - internal clock: counts on PCLK/1. */

	/* TPU1 Timer mode register */
	MTU1.TMDR1.BYTE	= 0x00;		/*	b7     ICSELD   - TGRD Input Capture Input Select - set to 0(not use input capture).
									b6     ICSELB   - TGRB Input Capture Input Select - set to 0(not use input capture).
									b5     BFB      - Buffer Operation B - operates normally .
									b4     BFA      - Buffer Operation A - operates normally .
									b3:b0  MD3:MD0  - Mode Select - Normal operation. */

	/* MTU1 Timer I/O Control Register */
	MTU1.TIOR.BYTE  = 0x00; 	/*  b7:b4  IOB3:IOB0 - TGRB Control - Output disabled.
									b3:b0  IOA3:IOA0 - TGRA Control - Output disabled. */

	/* MTU1 Timer Interrupt Enable Register */
	MTU1.TIER.BYTE	= 0x01;		/*  b7  TTGE     - A/D Conversion Start Request Enable - disabled.
									b6  Reserved - Set to 1.
									b5  TCIEU    - Underflow Interrupt Enable          - disabled.
									b4  TCIEV    - Overflow Interrupt Enable           - disabled.
									b3  TGIED    - TGRB Interrupt Enable               - disabled.
									b2  TGIEC    - TGRB Interrupt Enable               - disabled.
                                    b1  TGIEB    - TGRB Interrupt Enable               - disabled.
                                    b0  TGIEA    - TGRA Interrupt Enable               - enabled.  */

	/* Timer Counter Register */
	MTU1.TCNT	= 0x0000;		/* TCNT clear */

	/* Timer General Register A */
	MTU1.TGRA	= mtu_tgrx_set_table[sampling_rate];	/* set PCM Sampling Rate	*/

	/* Timer General Register B */
	MTU1.TGRB  = 0x0000;		/* TGRB not use.  */

	IEN(MTU1, TGIA1) = 0;
	IPR(MTU1, TGIA1) = 1;
	IR(MTU1, TGIA1) = 0;
	IEN(MTU1, TGIA1) = 1;

}

void decode_interval_timer_start(void)
{
	MTU.TSTRA.BIT.CST1 = 1;
}

void decode_interval_timer_stop(void)
{
	MTU.TSTRA.BIT.CST1 = 0;
}

void decode_interval_timer_sleep(void)
{
	IEN(MTU1, TGIA1) = 0;
	MTU.TSTRA.BIT.CST1 = 0;
	IPR(MTU1, TGIA1) = 0;
	IR(MTU1, TGIA1) = 0;

	SYSTEM.PRCR.WORD = 0xA500;
	SYSTEM.PRCR.WORD = 0xA502;
	MSTP_MTU = 1;
	SYSTEM.PRCR.WORD = 0xA500;
}


/* PWM setting */
void decode_pwm_init(void)
{
	/* P24 uses TIOC4A. */
	//PORT2.PMR.BIT.B4 = 1;
	PORTA.PMR.BIT.B2 = 1;


	MPC.PWPR.BYTE = 0x00;
	MPC.PWPR.BYTE;			/* Dummy read */
	MPC.PWPR.BYTE = 0x40;
	MPC.PWPR.BYTE;			/* Dummy read */
	//MPC.P24PFS.BYTE = 0x01;
	MPC.PA2PFS.BYTE = 0x08;
	MPC.PWPR.BYTE = 0x80;
	MPC.PWPR.BYTE;			/* Dummy read */


	POE3.POECR2.WORD = 0x0000;

	SYSTEM.PRCR.WORD = 0xA500;
	SYSTEM.PRCR.WORD = 0xA502;
	MSTP_MTU = 0;				/* MTU unit (MTU0 to MTU5) The module stop state is canceled. */
	SYSTEM.PRCR.WORD = 0xA500;

	/* Timer Read Write Enable Register */
	//MTU.TRWERA.BYTE = 0x01;		/*  b7:b1  Reserved - set to 0.
	MTU.TRWERB.BYTE = 0x01;		/*  b7:b1  Reserved - set to 0.
									b0     RWE      - Read Write Enable bit - Enabled.	*/

	/* MTU4 timer control register */
	//MTU4.TCR.BYTE = 0x40;		/*	b7:b5  CCLR2:CCLR0 - counter clear bit - TCNT counter cleared by TGRB compare match/input capture.
	MTU7.TCR.BYTE = 0x40;		/*	b7:b5  CCLR2:CCLR0 - counter clear bit - TCNT counter cleared by TGRB compare match/input capture.
									b4:b3  CKE1:CKE0   - clock select bit  - Counted at falling edge
									b2:b0  TPSC2:TPSC0 - mode select bit   - internal clock: counts on PCLK/1. */

	/* MTU4 timer mode register */
	//MTU4.TMDR1.BYTE = 0x12;		/*	b7     Reserved - set to 0.
	MTU7.TMDR1.BYTE = 0x12;		/*	b7     Reserved - set to 0.
									b6     Reserved - set to 0.
									b5     BFB      - Buffer Operation B - operates normally .
									b4     BFA      - Buffer Operation A - MTU4.TGRA and MTU4.TGRC used together for buffer operation.
									b3:b0  MD3:MD0  - Mode Select - PWM mode 1.*/

	//MTU4.TBTM.BIT.TTSA = 1;		/* TGRC to TGRA timing is TCNT clear */
	MTU7.TBTM.BIT.TTSA = 1;		/* TGRC to TGRA timing is TCNT clear */

	/* MTU Timer Output Master Enable Register */
	//MTU.TOERA.BIT.OE4A = 1;		/* TIOC4A output enable */
	MTU.TOERB.BIT.OE7A = 1;

	/* MTU4 Timer I/O Control Register */
	//MTU4.TIORH.BYTE = 0x65;		/*	b7:b4  IOB3:IOB0 - TGRB Control - Initial output is 1 output; 1 output at compare match.
	MTU7.TIORH.BYTE = 0x65;		/*	b7:b4  IOB3:IOB0 - TGRB Control - Initial output is 1 output; 1 output at compare match.
									b3:b0  IOA3:IOA0 - TGRA Control - Initial output is 1 output; 0 output at compare match. */
	//MTU4.TIORL.BYTE = 0x00;		/*	b7:b4  IOD3:IOD0 - TGRD Control - Output disabled.
	MTU7.TIORL.BYTE = 0x00;		/*	b7:b4  IOD3:IOD0 - TGRD Control - Output disabled.
									b3:b0  IOC3:IOC0 - TGRC Control - Output disabled. */

	/* MTU4 Timer Interrupt Enable Register */
	//MTU4.TIER.BYTE  = 0x00;		/*	b7  TTGE     - A/D Conversion Start Request Enable   - disabled.
	MTU7.TIER.BYTE  = 0x00;		/*	b7  TTGE     - A/D Conversion Start Request Enable   - disabled.
									b6  TTGE2    - A/D Conversion Start Request Enable 2 - disabled.
									b5  TCIEU    - Underflow Interrupt Enable            - disabled.
									b4  TCIEV    - Overflow Interrupt Enable             - disabled.
									b3  TGIED    - TGRB Interrupt Enable                 - disabled.
									b2  TGIEC    - TGRB Interrupt Enable                 - disabled.
									b1  TGIEB    - TGRB Interrupt Enable                 - disabled.
									b0  TGIEA    - TGRA Interrupt Enable                 - disabled.  */

	/* Timer Counter Register */
	//MTU4.TCNT		= 0x0000;		/* TCNT clear */
	MTU7.TCNT		= 0x0000;		/* TCNT clear */

	/* Timer General Register A */
	//MTU4.TGRA		= PWM_SAMPLING / 2;
	MTU7.TGRA		= PWM_SAMPLING / 2;

	/* Timer General Register B */
	//MTU4.TGRB		= PWM_SAMPLING;
	MTU7.TGRB		= PWM_SAMPLING;

	/* Timer General Register C */
	//MTU4.TGRC		= PWM_SAMPLING / 2;
	MTU7.TGRC		= PWM_SAMPLING / 2;


	/* Timer Read Write Enable Register */
	//MTU.TRWERA.BYTE = 0x00;		/*  b7:b1  Reserved - set to 0.
	MTU.TRWERB.BYTE = 0x00;		/*  b7:b1  Reserved - set to 0.
									b0     RWE      - Read Write Enable bit - disabled.	*/

}

void decode_pwm_start(void)
{
	//MTU.TSTRA.BIT.CST4 = 1;
	MTU.TSTRB.BIT.CST7 = 1;
}

void decode_pwm_stop(void)
{
	//MTU.TSTRA.BIT.CST4 = 0;
	MTU.TSTRB.BIT.CST7 = 0;
}

void decode_pwm_sleep(void)
{
	/*
		It is no need to set the module stop bit
		because PWM sleep is executed in decode_interval_timer_sleep().
	*/
}


/* D/A Converter setting */
void decode_da_converter_init(void)
{
	/* P03 uses DA0. */
	PORT0.PDR.BIT.B3 = 0;
	PORT0.PMR.BIT.B3 = 0;

	MPC.PWPR.BYTE = 0x00;
	MPC.PWPR.BYTE;			/* Dummy read */
	MPC.PWPR.BYTE = 0x40;
	MPC.PWPR.BYTE;			/* Dummy read */
	MPC.P03PFS.BYTE = 0x80;
	MPC.PWPR.BYTE = 0x80;
	MPC.PWPR.BYTE;			/* Dummy read */

	SYSTEM.PRCR.WORD = 0xA500;
	SYSTEM.PRCR.WORD = 0xA502;
	MSTP_DA = 0;			/* D/A Converter Module Stop state is canceled. */
	SYSTEM.PRCR.WORD = 0xA500;

	/* D/A Control Register */
	DA.DACR.BYTE = 0x1F;	/*  b7     DAOE1    - D/A Output Enable 1 - disabled.
								b6     DAOE0    - D/A Output Enable 0 - disabled.
								b5     DAE      - D/A Enable          - D/A conversion is independently controlled .
								b4:b0  Reserved - Set to 1. */

	/* D/A Data Placement Register */
	DA.DADPR.BYTE = 0x00;	/*	b7     DPSEL    - Data Placement Select - aligned to the RSB.
								b6:b0  Reserved - Set to 0.*/

	/* D/A Data Register */
	DA.DADR0 = 0x0800;		/* D/A Data initialized. */
}

void decode_da_converter_start(void)
{
	/* D/A Output Enable 0 - enabled. */
	DA.DACR.BIT.DAOE0 = 1;
}

void decode_da_converter_stop(void)
{
	/* D/A Output Enable 0 - disabled. */
	DA.DACR.BIT.DAOE0 = 0;
}

void decode_da_converter_sleep(void)
{
	DA.DACR.BYTE = 0x1F;
	SYSTEM.PRCR.WORD = 0xA500;
	SYSTEM.PRCR.WORD = 0xA502;
	MSTP_DA = 1;
	SYSTEM.PRCR.WORD = 0xA500;
}


