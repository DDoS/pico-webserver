#include <hardware/structs/watchdog.h>
#include <hardware/watchdog.h>

#include <pico/bootrom.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>

#include "encre_file.h"
#include "GDEP073E01.h"
#include "tusb_lwip_glue.h"

static void *current_connection;
static struct encre_file file;
static semaphore_t receive_file_semaphore;
static semaphore_t display_file_semaphore;

err_t httpd_post_begin(void *connection, const char *uri,
        const char *http_request, u16_t http_request_len,
        int content_len, char *response_uri,
        u16_t response_uri_len, u8_t *post_auto_wnd) {
    LWIP_UNUSED_ARG(http_request);
    LWIP_UNUSED_ARG(http_request_len);
    LWIP_UNUSED_ARG(content_len);
    LWIP_UNUSED_ARG(post_auto_wnd);

    snprintf(response_uri, response_uri_len, "/empty");
    if (current_connection) {
        return ERR_VAL;
    }

    static const char image[] = "/image";
    if (memcmp(uri, image, sizeof(image)) == 0 &&
            sem_acquire_timeout_ms(&receive_file_semaphore, 500)) {
        current_connection = connection;
        begin_encre_file(&file);
        return ERR_OK;
    }

    return ERR_VAL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    return continue_encre_file(&file, p) ? ERR_OK : ERR_VAL;
}

void httpd_post_finished(void *connection, char *response_uri,
        u16_t response_uri_len) {

    snprintf(response_uri, response_uri_len, "/empty");
    current_connection = NULL;
    sem_release(&display_file_semaphore);
}

void core1_entry() {
    while (true) {
        sem_acquire_blocking(&display_file_semaphore);
        if (file.read_colors) {
            GDEP073E01_write_image(file.colors);
        }
        sem_release(&receive_file_semaphore);
    }
}

int main() {
    // Initialize tinyusb, lwip, dhcpd and httpd
    init_lwip();
    wait_for_netif_is_up();
    dhcpd_init();
    httpd_init();

    sem_init(&receive_file_semaphore, 1, 1);
    sem_init(&display_file_semaphore, 0, 1);

    multicore_launch_core1(core1_entry);

    while (true) {
        tud_task();
        service_traffic();
    }

    return 0;
}
