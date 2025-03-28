
#ifndef __UTILS_H
#define __UTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

void dump_sockaddr_in (struct sockaddr_in * pInfo);

#endif
