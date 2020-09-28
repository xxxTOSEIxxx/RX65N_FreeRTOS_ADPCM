/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include "task_function.h"
#include "r_s2_peripheral_if.h"
#include "r_usb_hmsc_apl_config.h"
#include "r_usb_hmsc_apl.h"
#include "r_sci_rx_if.h"        // The SCI module API interface file.
#include "r_byteq_if.h"         // The BYTEQ module API interface file.
#include "r_sci_rx_config.h"    // User configurable options for the SCI module

#include "Global.h"
extern GLOBAL_INFO_TABLE			g_tGlobalInfo;
static sci_hdl_t   					g_sci_handle;
static uint8_t						g_szBuff[256 + 1];

static uint8_t						g_FileListNum;
static FILE_INFO_TABLE				g_FileList[10];



#define SOUND_FILE_PATH				( "sound.dat" )


void ErrorLed()
{
	PORTA.PODR.BIT.B1 = 1;
	vTaskDelay(200);
	PORTA.PODR.BIT.B1 = 0;
	vTaskDelay(200);
	PORTA.PODR.BIT.B1 = 1;
	vTaskDelay(200);
	PORTA.PODR.BIT.B1 = 0;
	vTaskDelay(200);
	PORTA.PODR.BIT.B1 = 1;
	vTaskDelay(200);
	PORTA.PODR.BIT.B1 = 0;
	vTaskDelay(500);
}

void sci_callback(void *pArgs)
{
	sci_cb_args_t   *args;

    args = (sci_cb_args_t *)pArgs;

    if (args->event == SCI_EVT_RX_CHAR)
    {
    }
    else if (args->event == SCI_EVT_RXBUF_OVFL)
    {
        /* From RXI interrupt; rx queue is full; 'lost' data is in args->byte
           You will need to increase buffer size or reduce baud rate */
        nop();
    }
    else if (args->event == SCI_EVT_OVFL_ERR)
    {
        /* From receiver overflow error interrupt; error data is in args->byte
           Error condition is cleared in calling interrupt routine */
        nop();
    }
    else if (args->event == SCI_EVT_FRAMING_ERR)
    {
        /* From receiver framing error interrupt; error data is in args->byte
           Error condition is cleared in calling interrupt routine */
        nop();
    }
    else if (args->event == SCI_EVT_PARITY_ERR)
    {
        /* From receiver parity error interrupt; error data is in args->byte
           Error condition is cleared in calling interrupt routine */
        nop();
    }
}


//-------------------------------------------------------------------
// メッセージ表示
//-------------------------------------------------------------------
void DispMessage(const char *pszMessage, uint8_t Size)
{
	sci_err_t   sci_err;

	do
	{
		sci_err = R_SCI_Send(g_sci_handle, pszMessage, Size);
	} while( sci_err == SCI_ERR_XCVR_BUSY);
}




//-------------------------------------------------------------------
// ファイルリスト作成
//-------------------------------------------------------------------
int GetFileList()
{
	DIR			dir;
	FRESULT		ret = FR_OK;
	FILINFO     tFileInfo;

	g_FileListNum = 0;
	memset(g_FileList,0x00,sizeof(g_FileList));


	ret = f_opendir(&dir, "");
	if (ret != FR_OK)
	{
		sprintf(g_szBuff,"f_opendir error. [ret:%d]\n",ret);
		DispMessage(g_szBuff, strlen(g_szBuff));
		return -1;
	}

	// エラー or 10ファイル分取得するまで
	while ( g_FileListNum < 10)
	{
		ret = f_readdir(&dir, &tFileInfo);
		if ((ret != FR_OK) || (strlen(tFileInfo.fname) == 0))
		{
			break;
		}

		if ((strcmp(tFileInfo.fname,".")==0) || (strcmp(tFileInfo.fname,"..")==0))
		{
			continue;
		}

		/* ディレクトリ */
        if (tFileInfo.fattrib & AM_DIR)
        {
        	continue;
        }

		g_FileList[g_FileListNum].Size = (UINT)tFileInfo.fsize;
		strncpy(g_FileList[g_FileListNum].szName, tFileInfo.fname, sizeof(tFileInfo.fname));
		g_FileListNum++;
	}

	f_closedir(&dir);

	return 0;
}



//-------------------------------------------------------------------
// ファイル選択
//-------------------------------------------------------------------
char SelectFile()
{
	int			iRet = 0;
	sci_err_t   sci_err;
	uint8_t     szSciBuff[5] = { 0x00 };
	double		SamplingRate = 0;

	sprintf(g_szBuff, "---[RX65N ADPCM Test ]--------------------------\n");
	DispMessage(g_szBuff, strlen(g_szBuff));

	if (g_tGlobalInfo.eUsbKind == USB_KIND_DISCONNECT)
	{
		sprintf(g_szBuff, "USB device is not connected!\n");
		DispMessage(g_szBuff, strlen(g_szBuff));
		sprintf(g_szBuff, "Please connect a USB device.\n");
		DispMessage(g_szBuff, strlen(g_szBuff));

	    // キー入力待ち
	    do
	    {
	        sci_err = R_SCI_Receive(g_sci_handle, szSciBuff, 1);
	    } while (SCI_ERR_INSUFFICIENT_DATA == sci_err);

	    szSciBuff[0] = '`';		// リトライ文字とする
	}
	else
	{
		sprintf(g_szBuff, " [OutputMode    : %s]　<-- Mode Change 'M'.\n", g_tGlobalInfo.eAdpcmMode == ADPCM_OUTPUT_MODE_DA ? "DA" : "PWM");
		DispMessage(g_szBuff, strlen(g_szBuff));

		switch(g_tGlobalInfo.eSamplingRate){
		case SAMPLING_RATE_KIND_08000:
			SamplingRate = 8.0f;
			break;
		case SAMPLING_RATE_KIND_11025:
			SamplingRate = 11.025f;
			break;
		case SAMPLING_RATE_KIND_16000:
			SamplingRate = 16.0f;
			break;
		case SAMPLING_RATE_KIND_22050:
			SamplingRate = 22.05f;
			break;
		case SAMPLING_RATE_KIND_32000:
			SamplingRate = 32.0f;
			break;
		case SAMPLING_RATE_KIND_44100:
			SamplingRate = 44.1f;
			break;
		}
		sprintf(g_szBuff, " [Sampling Rate : %2.3fKhz]　<-- Mode Change 'S'.\n", SamplingRate);
		DispMessage(g_szBuff, strlen(g_szBuff));

		sprintf(g_szBuff, "\n");
		DispMessage(g_szBuff, strlen(g_szBuff));

		// ファイルリスト作成
		iRet = GetFileList();
		if (iRet != 0)
		{
			sprintf(g_szBuff, "GetFileList Error!\n");
			DispMessage(g_szBuff, strlen(g_szBuff));
	    	while(1)
	    	{
	    		ErrorLed();
	    	};
		}

		// ファイル表示
		for (int i = 0 ; i < g_FileListNum ; i++)
		{
			sprintf(g_szBuff, " %d ... %s (%u)\n", i , g_FileList[i].szName, g_FileList[i].Size);
			DispMessage(g_szBuff, strlen(g_szBuff));
		}

		sprintf(g_szBuff, "Please select the file to play.\n");
		DispMessage(g_szBuff, strlen(g_szBuff));
		sprintf(g_szBuff, "==>");
		DispMessage(g_szBuff, strlen(g_szBuff));

	    // キー入力待ち
	    do
	    {
	        sci_err = R_SCI_Receive(g_sci_handle, szSciBuff, 1);
	    } while (SCI_ERR_INSUFFICIENT_DATA == sci_err);

	}

    return szSciBuff[0];
}


//-------------------------------------------------------------------
// サンプリングレート変更
//-------------------------------------------------------------------
void SelectSamplingRate()
{
	sci_err_t   sci_err;
	uint8_t     szSciBuff[5] = { 0x00 };

	sprintf(g_szBuff, "***********************************************\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "* Sampling Rate Select\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "***********************************************\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　1 ...  8.0Khz\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　2 ... 11.025Khz\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　3 ... 16.0Khz\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　4 ... 22.0Khz\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　5 ... 32.0Khz\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　6 ... 44.1Khz\n");
	DispMessage(g_szBuff, strlen(g_szBuff));

	while(1)
	{
		sprintf(g_szBuff, "==>");
		DispMessage(g_szBuff, strlen(g_szBuff));

		// キー入力待ち
		do
		{
			sci_err = R_SCI_Receive(g_sci_handle, szSciBuff, 1);
		} while (SCI_ERR_INSUFFICIENT_DATA == sci_err);

		if (szSciBuff[0] == '1')
		{
			g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_08000;
			break;
		}
		else if (szSciBuff[0] == '2')
		{
			g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_11025;
			break;
		}
		else if (szSciBuff[0] == '3')
		{
			g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_16000;
			break;
		}
		else if (szSciBuff[0] == '4')
		{
			g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_22050;
			break;
		}
		else if (szSciBuff[0] == '5')
		{
			g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_32000;
			break;
		}
		else if (szSciBuff[0] == '6')
		{
			g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_44100;
			break;
		}
	}
}


//-------------------------------------------------------------------
// モード変更
//-------------------------------------------------------------------
void SelectMode()
{
	sci_err_t   sci_err;
	uint8_t     szSciBuff[5] = { 0x00 };

	sprintf(g_szBuff, "***********************************************\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "* OutputMode Select\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "***********************************************\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　1 ... DA\n");
	DispMessage(g_szBuff, strlen(g_szBuff));
	sprintf(g_szBuff, "　2 ... PWM\n");
	DispMessage(g_szBuff, strlen(g_szBuff));

	while(1)
	{
		sprintf(g_szBuff, "==>");
		DispMessage(g_szBuff, strlen(g_szBuff));

		// キー入力待ち
		do
		{
			sci_err = R_SCI_Receive(g_sci_handle, szSciBuff, 1);
		} while (SCI_ERR_INSUFFICIENT_DATA == sci_err);

		if (szSciBuff[0] == '1')
		{
			g_tGlobalInfo.eAdpcmMode = ADPCM_OUTPUT_MODE_DA;
			break;
		}
		else if (szSciBuff[0] == '2')
		{
			g_tGlobalInfo.eAdpcmMode = ADPCM_OUTPUT_MODE_PWM;
			break;
		}
	}
}


void Task_Adpcm(void * pvParameters)
{
	FILINFO		tFileInfo;
    FIL         file;
    FRESULT     ret;
    sci_cfg_t   sci_config;
    sci_err_t   sci_err;
    uint8_t     szSciBuff[5] = { 0x00 };
    char		ch = 0x00;
    int			No = 0;

	g_tGlobalInfo.eAdpcmMode = ADPCM_OUTPUT_MODE_DA;
	g_tGlobalInfo.eSamplingRate = SAMPLING_RATE_KIND_22050;
	memset(g_szBuff, 0x00, sizeof(g_szBuff));

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC); // Use BSP function to unlock the MPC register.

    /* Initialize channel 2 TXD and RXD pins */
    PORT5.PODR.BIT.B0 = 0;      // Set P50(Pin56) signal state low (power-up default)
    MPC.P50PFS.BYTE = 0x0A;     // Assign P50(Pin56) to TXD2 function
    MPC.P52PFS.BYTE = 0x0A;     // Assign P52(Pin54)) to RXD2 function

    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC); // Re-lock the MPC register.

    PORT5.PDR.BIT.B0 = 1;       // Set P50(Pin56) direction to output (for use as TXD)
    PORT5.PDR.BIT.B2 = 0;       // Set P52(Pin54) direction to input (for use as RXD)
    PORT5.PMR.BIT.B0 = 1;       // Set P50(Pin56) mode to peripheral operation
    PORT5.PMR.BIT.B2 = 1;       // Set P52(Pin54) mode to peripheral operation

	// LED
	PORTA.PDR.BIT.B1 = 1;
	PORTA.PODR.BIT.B1 = 0;

    /* Set up the configuration data structure for asynchronous (UART) operation. */
    sci_config.async.baud_rate    = 9600;
    sci_config.async.clk_src      = SCI_CLK_INT;
    sci_config.async.data_size    = SCI_DATA_8BIT;
    sci_config.async.parity_en    = SCI_PARITY_OFF;
    sci_config.async.parity_type  = SCI_EVEN_PARITY;
    sci_config.async.stop_bits    = SCI_STOPBITS_1;
    sci_config.async.int_priority = 3;    // 1=lowest, 15=highest

    /* OPEN ASYNC CHANNEL
     *  Provide address of the config structure,
     *  the callback function to be assigned,
     *  and the location for the handle to be stored.*/
    sci_err = R_SCI_Open(SCI_CH2, SCI_MODE_ASYNC, &sci_config, sci_callback, &g_sci_handle);
    if (SCI_SUCCESS != sci_err)
     {
    	printf("R_SCI_Open Error. [sci_err:%d]\n", sci_err);
    	while(1)
    	{
    		ErrorLed();
    	};
     }


    // キー入力待ち
    do
    {
        sci_err = R_SCI_Receive(g_sci_handle, szSciBuff, 1);
    } while (SCI_ERR_INSUFFICIENT_DATA == sci_err);

    //--------------------------------------------------------------------------------
	while( 1 )
	{
		// ファイル選択
		ch = SelectFile();
		if (ch == '`')
		{
			continue;
		}

		if ((ch == 'M') || (ch == 'm'))
		{
			// モード変更
			SelectMode();
			continue;
		}
		else if ((ch == 'S') || (ch == 's'))
		{
			// サンプリングレート変更
			SelectSamplingRate();
			continue;
		}
		else if ((ch < '0') || (ch > '9'))
		{
			sprintf(g_szBuff, "Unknown character...\n");
			DispMessage(g_szBuff, strlen(g_szBuff));
			continue;
		}

		No = ch - '0';
		if (No > g_FileListNum)
		{
			sprintf(g_szBuff, "The file with the specified number does not exist...\n");
			DispMessage(g_szBuff, strlen(g_szBuff));
			continue;
		}

		// 音声ファイルオープン
		ret = f_open(&file, g_FileList[No].szName,(FA_READ | FA_OPEN_EXISTING));
		if (ret != 0)
		{
			sprintf(g_szBuff, "f_open Error! [ret:%d]\n",ret);
			DispMessage(g_szBuff, strlen(g_szBuff));
			continue;
		}

		PORTA.PODR.BIT.B1 = 1;

		sprintf((char *)g_szBuff, "Start playing. [%s (%u)]\n", g_FileList[No].szName, g_FileList[No].Size);
		DispMessage(g_szBuff, strlen(g_szBuff));

		// エンコード開始
		R_decode_main(&file,g_FileList[No].Size);

		f_close(&file);

		PORTA.PODR.BIT.B1 = 0;
	}
}
