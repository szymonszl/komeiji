#include <stdio.h>
#include <string.h>

#include "sock/url.h"
#include "utils/buffer.h"
#include "utils/string.h"

int main(int argc, char** argv) {
    url_t uri;
    url_parse("wss://secret.aroltd.com:17777", &uri);

    printf(base64_decode(base64_encode_str("")));

    return 0;
}