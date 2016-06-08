/*
 * ParsingData.c
 *
 *  Created on: 07 июня 2016 г.
 *      Author: logunov
 */

#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "user_config.h"
#include "n_user_main.h"
#include "cJSON.h"


extern  mSysSett GlSett;
extern tConnState connState;
extern struct espconn *pCon;
u8 USER_Debug = 0;

#define ESP_PARAM_START_SEC   0x3C

//************************
void ICACHE_FLASH_ATTR ChangeHostIP(char* strIp)
{
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	read_Sett(&GlSett);
	os_memset(GlSett.tcpserverip, 0, sizeof(GlSett.tcpserverip));
	os_sprintf(GlSett.tcpserverip,strIp);
	save_Sett(&GlSett);

}
//************************
void ICACHE_FLASH_ATTR ChangeNetMask(char* strMsk)
{
	//	u32 iAdr= ipaddr_addr(strMsk);
	read_Sett(&GlSett);
	os_memset(GlSett.NetMask, 0, sizeof(GlSett.NetMask));
	os_sprintf(GlSett.NetMask,strMsk);
	save_Sett(&GlSett);
}
//************************
void ICACHE_FLASH_ATTR ChangeLocIP(char* strLoc)
{
	//	u32 iAdr= ipaddr_addr(strMsk);
	read_Sett(&GlSett);
	os_memset(GlSett.locip, 0, sizeof(GlSett.locip));
	os_sprintf(GlSett.locip,strLoc);
	save_Sett(&GlSett);
}
//************************
void ICACHE_FLASH_ATTR ChangeGateW(char* strGW)
{
	read_Sett(&GlSett);
	os_memset(GlSett.gateW, 0, sizeof(GlSett.gateW));
	os_sprintf(GlSett.gateW,strGW);
	save_Sett(&GlSett);
}
//************************
void ICACHE_FLASH_ATTR ChangePort(u32 iPort)
{

	read_Sett(&GlSett);
	GlSett.tcpservport = iPort;
	save_Sett(&GlSett);
}
//************************
void ICACHE_FLASH_ATTR ChangeDebOnOff(u8 iVal)
{

	read_Sett(&GlSett);
	GlSett.dbgOnOff = iVal;
	save_Sett(&GlSett);
}
//************************
void ICACHE_FLASH_ATTR ChangeBRate(u32 iBRate)
{

	read_Sett(&GlSett);
	GlSett.baudRate = iBRate;
	save_Sett(&GlSett);
	system_restart();
}
//************************
void ICACHE_FLASH_ATTR ChangeSSID(char* tSSID)
{

	read_Sett(&GlSett);
	os_memset(GlSett.ssid, 0, sizeof(GlSett.ssid));
	os_sprintf(GlSett.ssid,tSSID);
	save_Sett(&GlSett);

}
//************************
void ICACHE_FLASH_ATTR ChangePass(char* tPass)
{
	read_Sett(&GlSett);
	os_memset(GlSett.pass, 0, sizeof(GlSett.pass));
	os_sprintf(GlSett.pass,tPass);
	save_Sett(&GlSett);
}
//************************
void ICACHE_FLASH_ATTR ChangeAllSett(mSysSett* tSett)
{
	//u32 tBrate = GlSett.baudRate;
	read_Sett(&GlSett);
	os_memset(&GlSett, 0, sizeof(GlSett));
	os_memcpy(&GlSett,tSett, sizeof(GlSett));
	save_Sett(&GlSett);
	// if(tBrate != GlSett.baudRate)
	system_restart();

}
/******************************************************************************
 * FunctionName : readSett
 * Description  :
 * Parameters   :
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
read_Sett(mSysSett* mSett)
{


	spi_flash_read((ESP_PARAM_START_SEC) * SPI_FLASH_SEC_SIZE,
			(uint32 *)mSett, sizeof(mSysSett));

	if(mSett->Flag != 0xAA)
	{
		os_memset(mSett,0,sizeof(mSysSett));
		os_sprintf(mSett->pass,"none");
		os_sprintf(mSett->ssid,"EmptyNet");
		os_sprintf(mSett->tcpserverip,"192.168.255.255");
		os_sprintf(mSett->gateW,"192.168.10.1");
		os_sprintf(mSett->NetMask,"255.255.255.0");
		os_sprintf(mSett->locip,"192.168.10.100");
		mSett->tcpservport = 1234;
		mSett->baudRate = 115200;
		mSett->Flag = 0xAA;
		mSett->dbgOnOff = 1;

		save_Sett(mSett);
	}
}

/******************************************************************************
 * FunctionName : save_Sett
 * Description  :
 *              :
 * Parameters   :
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
save_Sett(mSysSett *mSett)
{

	spi_flash_erase_sector(ESP_PARAM_START_SEC);
	spi_flash_write((ESP_PARAM_START_SEC) * SPI_FLASH_SEC_SIZE,
			(uint32 *)mSett, sizeof(mSysSett));

}

/******************************************************************************
 * FunctionName : save_Sett
 * Description  :
 *              :
 * Parameters   :
 * Returns      : none
 *******************************************************************************/

void ICACHE_FLASH_ATTR
mParsData(char* Buf, u8 sender)
{
	cJSON *json=cJSON_Parse(Buf);
	cJSON *jsChld;
	if (json)
	{
		jsChld = cJSON_GetObjectItem(json,"ESP_Sett");

		if((jsChld != NULL)&&(jsChld->type == cJSON_Object))
		{
			char buff[64];
			u8 fWriteAll =0;
			mSysSett tempSett;

			cJSON *jsSett = cJSON_GetObjectItem(jsChld,"writeALL");
			if((jsSett != NULL)&&(jsSett->type == cJSON_Number))
			{
				os_memset(&tempSett, 0, sizeof(mSysSett));
				fWriteAll = 1;
			}
			jsSett = cJSON_GetObjectItem(jsChld,"hostIP");
			if((jsSett != NULL)&&(jsSett->type == cJSON_String))
			{
				if(fWriteAll == 1)
				{
					os_sprintf(tempSett.tcpserverip,jsSett->valuestring);
				}
				else
					ChangeHostIP(jsSett->valuestring);
			}
			jsSett = cJSON_GetObjectItem(jsChld,"netMsk");
			if((jsSett != NULL)&&(jsSett->type == cJSON_String))
			{
				if(fWriteAll == 1)
				{
					os_sprintf(tempSett.NetMask,jsSett->valuestring);
				}
				else
					ChangeNetMask(jsSett->valuestring);
			}
			jsSett = cJSON_GetObjectItem(jsChld,"locIP");
			if((jsSett != NULL)&&(jsSett->type == cJSON_String))
			{
				if(fWriteAll == 1)
				{
					os_sprintf(tempSett.locip,jsSett->valuestring);
				}
				else
					ChangeLocIP(jsSett->valuestring);
			}

			jsSett = cJSON_GetObjectItem(jsChld,"gateW");
			if((jsSett != NULL)&&(jsSett->type == cJSON_String))
			{
				if(fWriteAll == 1)
				{
					os_sprintf(tempSett.gateW,jsSett->valuestring);
				}
				else
					ChangeGateW(jsSett->valuestring);
			}
			jsSett = cJSON_GetObjectItem(jsChld,"nSSID");
			if((jsSett != NULL)&&(jsSett->type == cJSON_String))
			{
				if(fWriteAll == 1)
				{
					os_sprintf(tempSett.ssid,jsSett->valuestring);
				}
				else
					ChangeSSID(jsSett->valuestring);
			}
			jsSett = cJSON_GetObjectItem(jsChld,"nPass");
			if((jsSett != NULL)&&(jsSett->type == cJSON_String))
			{
				if(fWriteAll == 1)
				{
					os_sprintf(tempSett.pass,jsSett->valuestring);
				}
				else
					ChangePass(jsSett->valuestring);
			}

			jsSett = cJSON_GetObjectItem(jsChld,"bRate0");
			if((jsSett != NULL)&&(jsSett->type == cJSON_Number))
			{
				if(!(IS_VALID_B_RATE(jsSett->valueint)))
				{
					jsSett->valueint = BIT_RATE_9600;
				}
				if(fWriteAll == 1)
					tempSett.baudRate = jsSett->valueint;
				else
					ChangeBRate(jsSett->valueint);

			}
			jsSett = cJSON_GetObjectItem(jsChld,"nPort");
			if((jsSett != NULL)&&(jsSett->type == cJSON_Number))
			{
				if(fWriteAll == 1)
				{
					tempSett.tcpservport = jsSett->valueint;
				}
				else
					ChangePort(jsSett->valueint);
			}
			jsSett = cJSON_GetObjectItem(jsChld,"reset");
			if((jsSett != NULL)&&(jsSett->type == cJSON_Number)&&(jsSett->valueint == 1))
			{
				system_restart();
			}
			jsSett = cJSON_GetObjectItem(jsChld,"dbgOn");
			if((jsSett != NULL)&&(jsSett->type == cJSON_Number))
			{


				if(fWriteAll == 1)
				{
					tempSett.dbgOnOff = jsSett->valueint;
				}
				else
					ChangeDebOnOff(jsSett->valueint);
				USER_Debug = GlSett.dbgOnOff;

			}

			if(fWriteAll == 1)
			{
				tempSett.Flag = 0xAA;
				ChangeAllSett(&tempSett);
			}

			read_Sett(&GlSett);
			ets_uart_printf("---Changed:\nSSID: %s\n PASSWORD: %s\n HOST IP %s\n PORT %d\n BAUDRATE %d\n GATEWAY %s\n NETMASK %s\n FLAG 0x%02x\r\n",
										GlSett.ssid, GlSett.pass,GlSett.tcpserverip,GlSett.tcpservport,
										GlSett.baudRate,GlSett.gateW,GlSett.NetMask,GlSett.Flag);

		}
		else // for other perpherals
		{
			size_t len =strlen(Buf);
			if(sender == WLAN){
				uart0_tx_buffer(Buf, len);
			}
			else if(sender == UART){
				if(connState == TCP_CONNECTED)
				{
					if(Buf[0] != 0){
						// Отправляем данные
						espconn_sent(pCon, Buf, len);
					}
				}
				else
				{
					if(USER_Debug)ets_uart_printf("Connection failed\r\n");
				}
			}
		}

		cJSON_Delete(json);
		json = NULL;
	}

}



