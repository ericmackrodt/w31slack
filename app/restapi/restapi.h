#ifndef RESTAPI_H
#define RESTAPI_H

#include "windows.h"

DWORD restapi_getChatRooms(char * ip, int port, LPSTR response, DWORD maxResponseLength);
DWORD restapi_sendMessageToChannel(char * ip, int port, char * channel, char * message, char * token, LPSTR response, DWORD maxResponseLength);
DWORD restapi_getChannelMessages(char * ip, int port, char * channel, int limit, char * token, LPSTR response, DWORD maxResponseLength);
DWORD restapi_getChannelList(char * ip, int port, char * token, LPSTR response, DWORD maxResponseLength);
DWORD restapi_getUsersList(char * ip, int port, char * token, LPSTR response, DWORD maxResponseLength);

#endif
