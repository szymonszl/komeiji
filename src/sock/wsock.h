#ifndef KMJ_WSOCK_H
#define KMJ_WSOCK_H

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #ifdef __MINGW32__
    #undef _WIN32_WINNT
    #define _WIN32_WINNT _WIN32_WINNT_WIN8
  #endif

  #include <winsock2.h>
  #include <ws2tcpip.h>

  #define __SOCK_T SOCKET
  #define __ADDR_T SOCKADDR_IN
#else
  #include <arpa/inet.h>
  #include <errno.h>
  #include <fcntl.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <unistd.h>

  #define __SOCK_T int
  #define __ADDR_T struct sockaddr_in
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define KMJ_SOCK_BUFLEN 2048

#define KMJ_SOCK_APPEND         1
#define KMJ_SOCK_APPEND_CLR     2
#define KMJ_SOCK_APPEND_IGN_PTR 4
#define KMJ_SOCK_BLOCK          8

typedef struct {
    __SOCK_T sock;
    __ADDR_T addr;
    int addr_len;

    int sock_open;
    SSL* ssl;

    char* append_ptr;
    char* append_loc;
    char buffer[KMJ_SOCK_BUFLEN];
} wsock_t;

int ssl_init(void);

wsock_t* wsock_create(const char* url);
void wsock_free(wsock_t*);

#endif
