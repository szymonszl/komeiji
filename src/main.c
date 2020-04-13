#include <stdio.h>

#include "sock/url.h"

int main(int argc, char** argv) {
    url_t uri;
    url_parse("wss://secret.aroltd.com:17777", &uri);

    return 0;
}