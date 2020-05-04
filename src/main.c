#include <stdio.h>
#include <string.h>
#include <time.h>

#include "sock/wsock.h"
#include "utils/buffer.h"
#include "utils/string.h"

int main(int argc, char** argv) {
    srand((unsigned)time(NULL));

    /*url_t uri;
    int a = url_parse("ws://secret.aroltd.com:17777/", &uri);*/

    wsock_open("ws://secret.aroltd.com:17777");

    return 0;
}