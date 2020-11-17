#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sock/wsock.h"
#include "utils/buffer.h"
#include "utils/string.h"

static struct {
    char *server;
    char *uid;
    char *auth;
    char prefix;
} config;

int main(int argc, char** argv) {

    // cute banner
    puts("===      PARSEE       ===\n"
         "==   based on KOMEIJI  ==\n"
         "=  because markov hard  =\n"
         "====                 ====\n");

    srand((unsigned)time(NULL));

    // config loading
    FILE* f = fopen("kmj.cfg", "r");
    char key[64];
    char val[256];
    while (fscanf(f, "%64[^ \n] %256[^\n]%*c", key, val) == 2) {
        if (key[0] == '#') continue; // ignore comments
        else if (0 == strcmp(key, "server")) config.server = strdup(val);
        else if (0 == strcmp(key, "uid")) config.uid = strdup(val);
        else if (0 == strcmp(key, "auth")) config.auth = strdup(val);
        else if (0 == strcmp(key, "prefix")) config.prefix = val[0];
        else fprintf(stderr, "Warning: unrecognized key \"%s\" in config\n", key);
    }
    fclose(f);

    buffer_t *ib, *ob;
    ib = buffer_create();
    ob = buffer_create();

    wsock_t *very_cute_socket;

    printf("Connecting to \"%s\"...\n", config.server);
    very_cute_socket = wsock_open(config.server);
    if (!very_cute_socket) {
            printf("wsock_open() returned NULL, exiting!\n");
            goto dealloc;
    }
    printf("Connected!\n");

    char buf[256];
    sprintf(buf, "1\t%s\t%s", config.uid, config.auth);
    buffer_write_str(ob, buf);
    wsock_send(very_cute_socket, ob);
    buffer_truncate(ob);
    printf("Login packet sent!\n");
    int count = 0;
    struct timespec lastts, ts;
    while (1) {
        if (wsock_recv(very_cute_socket, ib, 0) != 0) {
            char* recvd = buffer_read_str(ib);
            printf("Received chars: \"%s\"\n", recvd);
            buffer_truncate_to(ib, strlen(recvd));
            if (strstr(recvd, "]hello")) {
                sprintf(buf, "2\t%s\tyes hello", config.uid);
                buffer_write_str(ob, buf);
                wsock_send(very_cute_socket, ob);
                buffer_truncate(ob);
            }
            free(recvd);
        }
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if ((ts.tv_sec-lastts.tv_sec) > 10) {
            printf("\033[7mAAAAAAAAAAa\033[0m\n");
            sprintf(buf, "0\t%s", config.uid);
            buffer_write_str(ob, buf);
            wsock_send(very_cute_socket, ob);
            buffer_truncate(ob);
            count = 0;
            lastts = ts;
        }
        usleep(10000);
    }
    printf("Closing the very cute socket...\n");
    wsock_close(very_cute_socket);
dealloc:
    printf("Exiting!");
    buffer_free(ib);
    buffer_free(ob);
    return 0;
}
