#include "hardware/structs/watchdog.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"

#include "tusb_lwip_glue.h"

#define LED_PIN 25

static void *current_connection = NULL;

err_t httpd_post_begin(void *connection, const char *uri,
        const char *http_request, u16_t http_request_len,
        int content_len, char *response_uri,
        u16_t response_uri_len, u8_t *post_auto_wnd) {
    LWIP_UNUSED_ARG(http_request);
    LWIP_UNUSED_ARG(http_request_len);
    LWIP_UNUSED_ARG(content_len);
    LWIP_UNUSED_ARG(post_auto_wnd);

    snprintf(response_uri, response_uri_len, "/empty");

    static const char led[] = "/led";
    if (memcmp(uri, led, sizeof(led)) == 0) {
        if (current_connection != connection) {
            current_connection = connection;
            return ERR_OK;
        }
    }

    return ERR_VAL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p) {
    err_t ret;
    if (current_connection == connection) {
        int led_state = pbuf_try_get_at(p, 0);
        if (led_state >= 0) {
            led_state = led_state != '0';
            gpio_put(LED_PIN, led_state);
        }
        ret = ERR_OK;
    } else {
        ret = ERR_VAL;
    }

    pbuf_free(p);

    return ret;
}

void httpd_post_finished(void *connection, char *response_uri,
        u16_t response_uri_len) {

    snprintf(response_uri, response_uri_len, "/empty");

    if (current_connection == connection) {
        current_connection = NULL;
    }
}

int main() {
    // Initialize tinyusb, lwip, dhcpd and httpd
    init_lwip();
    wait_for_netif_is_up();
    dhcpd_init();
    httpd_init();

    // For toggle_led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        tud_task();
        service_traffic();
    }

    return 0;
}
