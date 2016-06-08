/*
 * n_user_main.c
 *
 *  Created on: 22 янв. 2015 г.
 *      Author: logunov
 */
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "user_config.h"
#include "mem.h"
#include "gpio.h"
#ifndef LWIP_DEBUG
#include "espconn.h"
#include <stdio.h>
#endif

#include "n_user_main.h"
//#include "cJSON.h"



extern int ets_uart_printf(const char *fmt, ...);
int (*console_printf)(const char *fmt, ...) = ets_uart_printf;
extern void RxUART0_task(os_event_t*e);
static void ScanEndCb(void *arg,STATUS tStat);
static void wifi_check_ip(void *arg);
static void ICACHE_FLASH_ATTR Ini_espconn();


extern u8 USER_Debug ;
struct espconn Conn;
esp_tcp ConnTcp;
static char macaddr[6];
ETSTimer WiFiLinker;
tConnState connState = WIFI_CONNECTING;
char GPIO_Time_Active;
char payload[2024];
struct espconn MyCon;
struct espconn *pCon;

#define tQUEUE_LEN 4
os_event_t *mQueue;
u8 Scan_SSID[16];
u8 Scan_bSSID[16];
struct scan_config mScanNets;
bool ourNetOnAir =0;
mSysSett GlSett;

const char* Ch_auth_mode[]= {
    "OPEN",
    "WEP",
    "WPA_PSK",
    "WPA2_PSK",
    "WPA_WPA2_PSK"
};
const char* Ch_StatConnect[]= {
    "STATION_IDLE",
    "STATION_CONNECTING",
    "STATION_WRONG_PASSWORD",
    "STATION_NO_AP_FOUND",
    "STATION_CONNECT_FAIL",
    "STATION_GOT_IP"};

void setup_wifi_st_mode(void)
{
	wifi_set_opmode((wifi_get_opmode() | STATION_MODE) & STATIONAP_MODE);
	struct station_config stconfig;
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	read_Sett(&GlSett);
	if (wifi_station_get_config(&stconfig))
	{
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
		os_sprintf(stconfig.ssid, "%s", GlSett.ssid);
		os_sprintf(stconfig.password, "%s", GlSett.pass);

		if (!wifi_station_set_config(&stconfig))
		{
			if(USER_Debug)ets_uart_printf("set station config fail!\r\n");
		}
	}
	wifi_station_connect();
	//wifi_station_dhcpc_start();
	//wifi_station_set_auto_connect(1);

	if(USER_Debug)ets_uart_printf("ESP8266 in STA mode configured.\n ssid:%s\npass: %s \r\n",stconfig.ssid,stconfig.password);
}
//************************************************


//Init function
void ICACHE_FLASH_ATTR user_init()
{
	read_Sett(&GlSett);
	USER_Debug = GlSett.dbgOnOff;
	// Инициализация uart0 и uart1
	uart_init(GlSett.baudRate , GlSett.baudRate);
	// Ждем 100 мсек.
	os_delay_us(100);
	// Вывод строки в uart о начале запуска
	uart0_tx_buffer("ESP8266 platform starting...\r\n\0");
	// Структура с информацией о конфигурации STA (в режиме клиента AP)
	struct station_config stationConfig;
	char info[150];
	if (wifi_get_opmode() != STATION_MODE)
	{
		if(USER_Debug)ets_uart_printf(
				"ESP8266 not in STATION mode, it will be restart in STATION mode...\r\n");
		wifi_set_opmode(STATION_MODE);
	}
	if (wifi_get_opmode() == STATION_MODE)
	{
		wifi_station_dhcpc_stop();
		wifi_station_set_auto_connect(0);
		wifi_get_macaddr(SOFTAP_IF, macaddr);
		// Для отладки выводим в uart данные о настройке режима STA
		ets_uart_printf("OPMODE: %u\n SSID: %s\n PASSWORD: %s\n LOC IP %s\nHOST IP %s\n PORT %d\n BAUDRATE %d\n GATEWAY %s\n NETMASK %s\n FLAG 0x%02X\n DBG %d\r\n",
				wifi_get_opmode(), GlSett.ssid, GlSett.pass,GlSett.locip,GlSett.tcpserverip,GlSett.tcpservport,
				GlSett.baudRate,GlSett.gateW,GlSett.NetMask,GlSett.Flag,GlSett.dbgOnOff);

		struct ip_info ipConfig;
//		// Получаем данные о сетевых настройках
//		wifi_get_ip_info(STATION_IF, &ipConfig);
//		ets_uart_printf("Curr IpConf IP " IPSTR "\n Mask " IPSTR "\n GatW " IPSTR "\n\r\n",
//				IP2STR(ipConfig.ip.addr),IP2STR(ipConfig.netmask.addr),IP2STR(ipConfig.gw.addr ));
//
		int ip = ipaddr_addr(GlSett.locip);
		//os_memcpy(ipConfig.ip.addr, &ip, 4);
		ipConfig.ip.addr = ip;
		wifi_set_ip_info(STATION_IF, &ipConfig);



	}
	// Запускаем таймер проверки соединения по Wi-Fi, проверяем соединение раз в 1 сек., если соединение установлено, то запускаем TCP-клиент и отправляем тестовую строку.
	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *) wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 2000, 0);

	// Выводим сообщение о успешном запуске
	if(USER_Debug)ets_uart_printf("ESP8266 platform started!\r\n");

	mQueue = (os_event_t*)os_malloc(sizeof(os_event_t)*tQUEUE_LEN);
	system_os_task(RxUART0_task, USER_TASK_PRIO_0, mQueue, tQUEUE_LEN);
}
//****************************

static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{


	// Структура с информацией о полученном, ip адресе клиента STA, маске подсети, шлюзе.
	struct ip_info ipConfig;
	uint32_t mWiFiLinkerTime =2000;;
	// Отключаем таймер проверки wi-fi
	os_timer_disarm(&WiFiLinker);

	uint8 tWFStat = wifi_station_get_connect_status();
	if(USER_Debug)ets_uart_printf("-WiFi check. Status %s-\n",Ch_StatConnect[tWFStat]);
	// старт
	if (tWFStat == STATION_IDLE)  //STATION_CONNECTING STATION_GOT_IP
	{
		if(USER_Debug)ets_uart_printf("-Scan Air-\n");
		ourNetOnAir = 0;
		wifi_station_scan(NULL, ScanEndCb);
		goto end_wifi_chek;
	}
	else if (tWFStat == STATION_GOT_IP)
	{
//		wifi_get_ip_info(STATION_IF, &ipConfig);
//		if(ipConfig.ip.addr != 0)
//		{
			Ini_espconn();
			goto end_wifi_chek;
//		}
	}


	read_Sett(&GlSett);
	if(USER_Debug)ets_uart_printf("OPMODE: %u\n SSID: %s\n PASSWORD: %s\n HOST IP %s\n PORT %d\n BAUDRATE %d\n GATEWAY %s\n NETMASK %s\n FLAG 0x%02X\r\n",
			wifi_get_opmode(), GlSett.ssid, GlSett.pass,GlSett.tcpserverip,GlSett.tcpservport,
			GlSett.baudRate,GlSett.gateW,GlSett.NetMask,GlSett.Flag);

	os_timer_setfn(&WiFiLinker, (os_timer_func_t *) wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, mWiFiLinkerTime, 0);
	end_wifi_chek:
	if(USER_Debug)ets_uart_printf("------------ENd of WiFi check------------\n\r");

}
//******************************************************


static void ICACHE_FLASH_ATTR ScanEndCb(void *arg,STATUS tStat)
{
	if(USER_Debug)ets_uart_printf("-Scan calback stat: %d \n",tStat);
	os_timer_disarm(&WiFiLinker);

	uint8 ssid[33];
	char temp[128];
	uint32_t mWiFiLinkerTime;
	if (tStat == OK)
	{
		struct station_config WiFIStatConf;
		wifi_station_get_config(&WiFIStatConf);

		read_Sett(&GlSett);

		struct bss_info *bss_link = (struct bss_info *)arg;
		bss_link = bss_link->next.stqe_next;//ignore first

		while (bss_link != NULL)
		{
			os_memset(ssid, 0, 33);
			if (os_strlen(bss_link->ssid) <= 32)
			{
				os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
			}
			else
			{
				os_memcpy(ssid, bss_link->ssid, 32);
			}
			if(!(os_strcmp(ssid,GlSett.ssid)))
			{
				ourNetOnAir =1;
				if(USER_Debug)
					ets_uart_printf("found our ssid \"%s\"\n auth: %s  ,rssi: %ddb \r\n",
							ssid,Ch_auth_mode[ssid,bss_link->authmode],  bss_link->rssi);
				setup_wifi_st_mode();
				mWiFiLinkerTime = 4000;
				break;
			}
			bss_link = bss_link->next.stqe_next;
		}
		if(ourNetOnAir == 0) {
			if(USER_Debug)ets_uart_printf(" Our ssid \"%s\" not found\r\n",GlSett.ssid);
			mWiFiLinkerTime = 2000;
		}

	}
	else
	{

		if(USER_Debug)ets_uart_printf("\nERROR Scan\r\n");
		//at_backError;
	}
	os_timer_arm(&WiFiLinker, mWiFiLinkerTime, 0);
}


static void ICACHE_FLASH_ATTR Ini_espconn()
{
	char info[150];

	os_timer_disarm(&WiFiLinker);
	struct ip_info ipConfig;
	pCon = &MyCon;//(struct espconn *) os_zalloc(sizeof(struct espconn));
	if (pCon == NULL)
	{
		if(USER_Debug)ets_uart_printf("TCP connect failed\r\n");
		return;
	}
	pCon->type = ESPCONN_TCP;
	pCon->state = ESPCONN_NONE;
	// Задаем адрес TCP-сервера, куда будем отправлять данные
	//os_sprintf(tcpserverip, "%s", TCPSERVERIP);
	uint32_t ip = ipaddr_addr(GlSett.tcpserverip);

	pCon->proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));
	pCon->proto.tcp->local_port = espconn_port();
	// Задаем порт TCP-сервера, куда будем отправлять данные
	pCon->proto.tcp->remote_port = GlSett.tcpservport;
	os_memcpy(pCon->proto.tcp->remote_ip, &ip, 4);
	ip = ipaddr_addr(GlSett.locip);
	os_memcpy(pCon->proto.tcp->local_ip, &ip, 4);


	// Регистрируем callback функцию, вызываемую при установки соединения
	espconn_regist_connectcb(pCon, at_tcpclient_connect_cb);
	// Вывод отладочной информации
	if(USER_Debug)ets_uart_printf("Start esp connect  to " IPSTR ":%d\r\n",
			IP2STR(pCon->proto.tcp->remote_ip), pCon->proto.tcp->remote_port);
	// Установить соединение с TCP-сервером
	connState = TCP_CONNECTING;
	espconn_connect(pCon);
	os_timer_arm(&WiFiLinker, 5000, 0);
}



static void ICACHE_FLASH_ATTR at_tcpclient_connect_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *) arg;
	if(pespconn == pCon)
	{
		os_timer_disarm(&WiFiLinker);

//		ets_uart_printf("--Local " IPSTR ":%d  conected to host " IPSTR ":%d\r\n",
//				IP2STR(pCon->proto.tcp->local_ip),pCon->proto.tcp->local_port,
//				IP2STR(pCon->proto.tcp->remote_ip), pCon->proto.tcp->remote_port);

		ets_uart_printf("{\"connect\":{\"stat\":1,\"remoteIP\":\"" IPSTR "\",\"remotePort\":%d}}\r\n",
		IP2STR(pCon->proto.tcp->remote_ip), pCon->proto.tcp->remote_port);

		connState = TCP_CONNECTED;
		// callback функция, вызываемая после отключения
		espconn_regist_disconcb(pCon, at_tcpclient_discon_cb);
		//************************************************
		// callback функция, вызываемая после отправки данных
		espconn_regist_sentcb(pCon, at_tcpclient_sent_cb);
		espconn_regist_recvcb(pCon, tcpclient_recv_cb);

	}

}

static void ICACHE_FLASH_ATTR at_tcpclient_sent_cb(void *arg)
{
	struct espconn *pespconn = (struct espconn *) arg;
	if(pespconn == pCon)
	{
		//if(USER_Debug)ets_uart_printf("Send suseful\r\n");
		// Данные отправлены, отключаемся от TCP-сервера
		//espconn_disconnect(pespconn);
	}
}

static void ICACHE_FLASH_ATTR tcpclient_recv_cb(void *arg, char *pdata, unsigned short len)
{
	if(USER_Debug)ets_uart_printf("--------recive_cb--------\r\n");
	struct espconn *pespconn = (struct espconn *) arg;
	if(pespconn == pCon)
	{
		mParsData(pdata,WLAN);

	}
}

static void ICACHE_FLASH_ATTR at_tcpclient_discon_cb(void *arg)
{

	struct espconn *pespconn = (struct espconn *) arg;
	if(pespconn == pCon)
	{
		os_timer_disarm(&WiFiLinker);
		// Отключились, освобождаем память
		os_free(pespconn->proto.tcp);
		os_free(pespconn);

		connState = TCP_DISCONNECTED;
		//if(USER_Debug)ets_uart_printf("Disconnect callback\r\n");
		ets_uart_printf("{\"connect\":{\"stat\":0}}\r\n");
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *) wifi_check_ip, NULL);
		os_timer_arm(&WiFiLinker, 1000, 0);
	}
}







