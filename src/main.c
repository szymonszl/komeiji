#include <stdio.h>
#include <string.h>

#include "sock/url.h"
#include "utils/buffer.h"

int main(int argc, char** argv) {
    url_t uri;
    url_parse("wss://secret.aroltd.com:17777", &uri);

    char test[] = "one time i had a dream about a squid and it was so !";
    buffer_t* buf = buffer_create();
    buffer_write_str(buf, test);
    buffer_write_str(buf, " VERY ");
    buffer_truncate_to(buf, 9);
    buffer_write_str(buf, "GOODE");
    printf(buffer_read_str(buf));

    return 0;
}