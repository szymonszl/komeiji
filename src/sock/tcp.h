#ifndef KMJ_TCP_H
#define KMJ_TCP_H

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

#include "utils/buffer.h"

#define KMJ_TCP_CHUNK 2048

#define KMJ_TCP_NONE      0
#define KMJ_TCP_NO_BLOCK  1
#define KMJ_TCP_ONE_CHUNK 2
#define KMJ_TCP_WHOLE     4

#define KMJ_TCP_NO_SSL 0
#define KMJ_TCP_SSL    1

typedef struct {
    __SOCK_T sock;
    __ADDR_T addr;
    int addr_len;

    int sock_open;
    SSL* ssl;
} tcp_t;

tcp_t* tcp_open(const char* host, uint16_t port, int secure);

int tcp_send(tcp_t*, buffer_t*);
int tcp_send_raw(tcp_t*, const char* data, int length);

int tcp_recv(tcp_t*, buffer_t*, int flags);
int tcp_recv_to(tcp_t*, buffer_t*, int length, int flags);
int tcp_recv_raw(tcp_t*, char* data, int length, int flags);

int tcp_is_open(tcp_t*);
int tcp_is_secure(tcp_t*);
int tcp_is_data_ready(tcp_t*);

void tcp_close(tcp_t*);

#endif
