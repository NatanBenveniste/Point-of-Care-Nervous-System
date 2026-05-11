#include <stdio.h>
#include <string>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

int main() {

    stdio_init_all();
    cyw43_arch_init();

    sleep_ms(2000); // give USB time to connect

    printf("Enter commands: on, off, blink\n");

    std::string command;
    bool autoBlink = false;

    absolute_time_t lastBlink = get_absolute_time();
    bool ledState = false;

    while (true) {

        // read character if available
        int c = getchar_timeout_us(0);

        if (c != PICO_ERROR_TIMEOUT) {

            if (c == '\n' || c == '\r') {

                if (command == "on") {
                    autoBlink = false;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    printf("LED ON\n");
                }
                else if (command == "off") {
                    autoBlink = false;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    printf("LED OFF\n");
                }
                else if (command == "blink") {
                    autoBlink = true;
                    printf("Blink ON\n");
                }

                command.clear();

            } else {
                command += (char)c;
            }
        }

        // non-blocking blink
        if (autoBlink) {
            if (absolute_time_diff_us(lastBlink, get_absolute_time()) > 1000000) {

                ledState = !ledState;

                cyw43_arch_gpio_put(
                    CYW43_WL_GPIO_LED_PIN,
                    ledState
                );

                lastBlink = get_absolute_time();
            }
        }

        sleep_ms(10);
    }
}