#include <stdio.h>
#include <string.h>
#include "uartium.h"

int main(void)
{
    char buffer[256];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };
    uartium_status_t st = uartium_init(&cfg);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_init failed: %d\n", (int)st);
        return 1;
    }

    /* Buffer some messages (should accumulate in buffer) */
    st = uartium_buffer_message(UARTIUM_EVENT_INFO, "Info message");
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "buffer_message info failed: %d\n", (int)st); return 1; }

    st = uartium_buffer_message(UARTIUM_EVENT_WARNING, "Warning occurred");
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "buffer_message warning failed: %d\n", (int)st); return 1; }

    st = uartium_buffer_message(UARTIUM_EVENT_DEBUG, "Debugging");
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "buffer_message debug failed: %d\n", (int)st); return 1; }

    st = uartium_buffer_message(UARTIUM_EVENT_ERROR, "Something bad");
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "buffer_message error failed: %d\n", (int)st); return 1; }

    const uint8_t *bufptr = NULL;
    size_t buflen = 0;
    st = uartium_get_buffer(&bufptr, &buflen);
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "get_buffer failed: %d\n", (int)st); return 1; }

    printf("--- Buffered Messages (len=%zu) ---\n", buflen);
    if (buflen == 0 || bufptr[0] == '\0') printf("<empty>\n");
    else {
        printf("%s\n", (const char*)bufptr);
        printf("--- raw bytes ---\n");
        for (size_t i = 0; i < buflen; ++i) printf("%02X ", bufptr[i]);
        printf("\n");
    }

    /* Now use log_message which should flush after printing */
    st = uartium_log_message(UARTIUM_EVENT_INFO, "Logged info");
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "log_message info failed: %d\n", (int)st); return 1; }

    st = uartium_log_message(UARTIUM_EVENT_ERROR, "Logged error");
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "log_message error failed: %d\n", (int)st); return 1; }

    printf("--- After log_message (buffer should be cleared) ---\n");
    st = uartium_get_buffer(&bufptr, &buflen);
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "get_buffer failed: %d\n", (int)st); return 1; }
    printf("len=%zu, first=0x%02X\n", buflen, (unsigned)(bufptr[0]));

    st = uartium_deinit();
    if (st != UARTIUM_STATUS_OK) { fprintf(stderr, "deinit failed: %d\n", (int)st); return 1; }

    return 0;
}
