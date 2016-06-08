/*
 * n_user_main.h
 *
 *  Created on: 23 џэт. 2015 у.
 *      Author: logunov
 */

#ifndef USER_N_USER_MAIN_H_
#define USER_N_USER_MAIN_H_

typedef struct _mSysSett
{
	char  gateW[32] ;
	char  NetMask[32] ;
	char tcpserverip[32] ;
	u32  tcpservport;
	char ssid[16];
	char pass[32];
	u32 baudRate;
	u8 Flag;
	u8 dbgOnOff;
	char locip[32] ;


} mSysSett;
typedef enum {
	WIFI_CONNECTING,
	WIFI_CONNECTING_ERROR,
	WIFI_CONNECTED,
	TCP_DISCONNECTED,
	TCP_CONNECTING,
	TCP_CONNECTING_ERROR,
	TCP_CONNECTED,
	TCP_SENDING_DATA_ERROR,
	TCP_SENT_DATA
} tConnState;



static void ICACHE_FLASH_ATTR at_tcpclient_connect_cb(void *arg);
static void ICACHE_FLASH_ATTR at_tcpclient_discon_cb(void *arg);
static void ICACHE_FLASH_ATTR BtnTimerCb(void *arg) ;
static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg) ;
static void ICACHE_FLASH_ATTR at_tcpclient_sent_cb(void *arg);
static void ICACHE_FLASH_ATTR tcpclient_recv_cb(void *arg, char *pdata, unsigned short len);

static void ICACHE_FLASH_ATTR Ini_espconn();
static void ICACHE_FLASH_ATTR BtnTimerCb(void *arg) ;




#endif /* USER_N_USER_MAIN_H_ */

