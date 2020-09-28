/*
 * Global.h
 *
 *  Created on: 2020/09/27
 *      Author: MIBC
 */

#ifndef FRTOS_SKELETON_GLOBAL_H_
#define FRTOS_SKELETON_GLOBAL_H_

#include "r_stdint.h"

// USB接続種別
typedef enum
{
	USB_KIND_DISCONNECT = 0,		// USB未接続
	USB_KIND_CONNECT = 1,			// USB接続

} USB_KIND_ENUM;

// ADPCM出力種別
typedef enum
{
	ADPCM_OUTPUT_MODE_PWM = 0,		// PWM
	ADPCM_OUTPUT_MODE_DA = 1,		// DA
} ADPCM_OUTPUT_MODE_ENUM;


// サンプリングレート種別
typedef enum
{
	SAMPLING_RATE_KIND_08000 = 0,
	SAMPLING_RATE_KIND_11025 = 1,
	SAMPLING_RATE_KIND_16000 = 2,
	SAMPLING_RATE_KIND_22050 = 3,
	SAMPLING_RATE_KIND_32000 = 4,
	SAMPLING_RATE_KIND_44100 = 5
} SAMPLING_RATE_ENUM;



typedef struct
{
	uint8_t							szName[32];
	uint32_t						Size;
} FILE_INFO_TABLE;


typedef struct
{
	USB_KIND_ENUM 					eUsbKind;			// USB接続種別


	ADPCM_OUTPUT_MODE_ENUM 			eAdpcmMode;
	SAMPLING_RATE_ENUM 				eSamplingRate;


} GLOBAL_INFO_TABLE;










#endif /* FRTOS_SKELETON_GLOBAL_H_ */
