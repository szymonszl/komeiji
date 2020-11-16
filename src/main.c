#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sock/wsock.h"
#include "utils/buffer.h"
#include "utils/string.h"

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));
    char *addr = "ws://echo.websocket.org";

    buffer_t *ib, *ob;
    ib = buffer_create();
    ob = buffer_create();

    wsock_t *very_cute_socket;
    printf("Made a very_cute_socket\n");

    printf("Connecting to \"%s\"...\n", addr);
    very_cute_socket = wsock_open(addr);
    if (!very_cute_socket) {
            printf("wsock_open() returned NULL, exiting!\n");
            goto dealloc; // fight me
    }
    printf("Connected!\n");

    char str[16];
    for (int i = 0; i < 20; i++) {
        sprintf(str, "ping%02d", i);
        buffer_write_str(ob, str);
        wsock_send(very_cute_socket, ob);
        buffer_truncate(ob); // why doesnt this happen automatically is beyond me
        printf("[%i] Sent \"%s\"... ", i, str);
        fflush(stdout);

        wsock_recv(very_cute_socket, ib);
        char* recvd = buffer_read_str(ib);
        printf("Received chars: \"%s\"\n", recvd);
        buffer_truncate_to(ib, strlen(recvd)); // as above
        free(recvd);
        usleep(500000);
    }
    printf("Closing the very cute socket...\n");
    wsock_close(very_cute_socket);
dealloc:
    printf("Exiting!");
    buffer_free(ib);
    buffer_free(ob);
    return 0;
}
