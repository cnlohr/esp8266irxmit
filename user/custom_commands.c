//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>
#include "esp82xxutil.h"
extern int setcode;
extern int udpcommanding;
int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len) {
	char * buffend = buffer;

	switch( pusrdata[1] ) {
		// Custom command test
		case 'C': case 'c':
			buffend += ets_sprintf( buffend, "CC" );
        	printf("CC");
			return buffend-buffer;
		break;
		case 'I': case 'i':
			setcode = safe_atoi( &pusrdata[2]);
			buffend += ets_sprintf( buffend, "CI\t%d", setcode );
			udpcommanding = 255;
			return buffend - buffer;
		// Echo to UART
		case 'E': case 'e':
			if( retsize <= len ) return -1;
			ets_memcpy( buffend, &(pusrdata[2]), len-2 );
			buffend[len-2] = '\0';
			printf( "%s\n", buffend );
			return len-2;
		break;
	}

	return -1;
}
