// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

//#include <windows.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <time.h>
//#include <unistd.h>     /* for close() */

#ifdef WIN32// VERSION FOR WINDOWS
//#include <ws2tcpip.h>
//#include <winsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else // VERSION FOR LINUX
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/isiec.h>  /* for sockaddr_in and inet_addr() */
#endif

#include  "siec.h"
#include  "obiekty.h"
#include "kwaternion.h"
#include <math.h>
#include "wektor.h"


// TODO: reference additional headers your program requires here
