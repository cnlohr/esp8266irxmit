//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "commonservices.h"
#include "vars.h"
#include <mdns.h>
#include "ir_enc.h"

#define procTaskPrio        0
#define procTaskQueueLen    1

static volatile os_timer_t some_timer;
static struct espconn *pUdpServer;
usr_conf_t * UsrCfg = (usr_conf_t*)(SETTINGS.UserData);

//int ICACHE_FLASH_ATTR StartMDNS();


void user_rf_pre_init(void) { /*nothing*/ }

//Tasks that happen all the time.

os_event_t    procTaskQueue[procTaskQueueLen];

int setcode = 0;

#if 0 //IR Codes for one type of chinese LED
//208 = color advance.
//128 = Hose it (dim)
//184 56 88 120  = ??? 
//216 = ???
//64   = OFF
uint8_t setcodes[] = 
{
	160, //GREEN!
	8, //Yellow
	40, //Amber-yellow
	48, // Orange
	32, //RED (same as 232  -- 232 = fade?)
	16, //Pink-blue
	72, //PINK
	104, //inbetween
	112, 
	24, //Almost blue  (idental to 80)
	96, //BLUE
	136, //Cyan (duplicated as 152)
	176, //inbetewen (same as 168)
	144, //Almost green
};
#else
//0, 32, 56, 120, 128, 184, 192, 224, 248 = unused
//208 = white
//200,216 = continuously fade.
//232,240 = color jump.
//96 = BLACKifies it.  Permablack  224 turns them on.
uint8_t setcodes[] = 
{
	136, //YELLOW
	152, // ORANGE
	168,
	176,
	144, //RED
	72, // PURPLE
	88,
	104,
	112,
	80, //BLUE
	8, // Cyan
	24,
	40,
	48, 
	16, // GREEN
	208,
};
uint8_t setcodes_color[] = 
{
	255,255,0, //YELLOW
	255,191,0, // ORANGE
	255,127,0,
	255,63,0,
	255,0,0, //RED
	255,0,63, // PURPLE
	255,0,127,
	255,0,191,
	255,0,255,
	0,0, 255, //BLUE
	0,63,255, // Cyan
	0,127,255,
	0,191,191,
	0,255,127, 
	0,255,0, // GREEN
	255,255,255,
};


#endif

int udpcommanding  = 0;
static void ICACHE_FLASH_ATTR sendcode( int setcode );
static void ICACHE_FLASH_ATTR emitcolor( int r, int g, int b )
{
	if( r < 25 && g < 25 && b < 25 ) return;
	int ic = 0;
	int matchingcolor = -1;
	int matchinglevel = 90000000;
	for( ic = 0; ic < sizeof( setcodes_color )/3; ic++ )
	{
		int mr = setcodes_color[ic*3+0];
		int mg = setcodes_color[ic*3+1];
		int mb = setcodes_color[ic*3+2];
		int diff = (r - mr)*(r-mr) + (g-mg)*(g-mg) + (b-mb)*(b-mb);
		if( diff < matchinglevel )
		{
			matchinglevel = diff;
			matchingcolor = ic;
		}
	}
	sendcode( setcodes[matchingcolor] );
}

static void ICACHE_FLASH_ATTR sendcode( int setcode )
{
	uint8_t event[] = {
		0b00000000,
		0b11111111,
		0b00100000,
		0b11011111,
	};

	
	int send = setcode;
	if( setcode == -1 )
	{
		static int r = 0;
		send = setcodes[r];
		r++;
		if( r == sizeof(setcodes) ) r = 0;
	}

	event[2] = send;
	event[3] = ~send;

	ir_push( event, sizeof( event ) );
}


static void ICACHE_FLASH_ATTR procTask(os_event_t *events)
{
	if( ir_pending == 0 && udpcommanding==255 )
	{
		printf( "ASDF\n" );
		sendcode( setcode );
	}

	CSTick( 0 );
	system_os_post(procTaskPrio, 0, 0 );
}

//Timer event.
static void ICACHE_FLASH_ATTR myTimer(void *arg)
{
	CSTick( 1 ); // Send a one to uart
}


//Called when new packet comes in.
static void ICACHE_FLASH_ATTR
udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;

	udpcommanding  = pusrdata[0];
	if( ir_pending == 0 )
	{
		printf( "Codes: %d %d %d\n", pusrdata[1], pusrdata[3], pusrdata[2] );
		emitcolor( pusrdata[1], pusrdata[3], pusrdata[2] );
		uart0_sendStr("X");
	}
}

void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	uart0_sendStr("\r\nesp82XX Web-GUI\r\n" VERSSTR "\b");

//Uncomment this to force a system restore.
//	system_restore();

	CSSettingsLoad( 0 );
	CSPreInit();

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = COM_PORT;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);

	ir_init();

	if( espconn_create( pUdpServer ) )
	{
		while(1) { uart0_sendStr( "\r\nFAULT\r\n" ); }
	}

	CSInit();

	SetServiceName( "espcom" );
	AddMDNSName(    "esp82xx" );
	AddMDNSName(    "espcom" );
	AddMDNSService( "_http._tcp",    "An ESP82XX Webserver", WEB_PORT );
	AddMDNSService( "_espcom._udp",  "ESP82XX Comunication", COM_PORT );
	AddMDNSService( "_esp82xx._udp", "ESP82XX Backend",      BACKEND_PORT );

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 60, 1);

	printf( "Boot Ok.\n" );

	wifi_set_sleep_type(LIGHT_SLEEP_T);
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);

	system_os_post(procTaskPrio, 0, 0 );
}


//There is no code in this project that will cause reboots if interrupts are disabled.
void EnterCritical() { }

void ExitCritical() { }


