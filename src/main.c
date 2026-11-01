#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define BTN_ON 0
#define BTN_OFF 1
#define BTN_PIN 8
#define BTN_DEBOUNCE 50

#define TASK 4 // from 1 to 5

#if TASK == 1
volatile uint16_t pressCounter = 0;
uint16_t lastPressCounter = 0;
#endif

#if TASK > 1
uint16_t pressCounter = 0;
#endif

volatile bool btnPressedFlag = false;
#if TASK == 2
uint32_t lastPressedAtMs = 0;
#endif

#if TASK < 4
static void IRAM_ATTR gpio_isr_handler(void *arg) {
    #if TASK == 1
    pressCounter++;
    #endif

    #if TASK > 1
    btnPressedFlag = true;
    #endif
}
#endif

#if TASK > 2
typedef struct {
    bool raw;
    bool isPressed;
    uint32_t lastPressedAtMs;
} Button;
bool isBtnActive = false;
#endif


void init() {
    gpio_config_t in_conf = {
        #if TASK < 4
        .intr_type = GPIO_INTR_NEGEDGE,
        #else
        .intr_type = GPIO_INTR_DISABLE,
        #endif
        .pin_bit_mask = ((1ULL << BTN_PIN)),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&in_conf);

    #if TASK < 4
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, gpio_isr_handler, NULL);
    #endif
}

void logPressCount() {
    printf("pressCount: %d\n", pressCounter);
}

#if TASK > 2
bool handleButtonUpdate(Button *b, uint32_t now) {
    bool raw = gpio_get_level(BTN_PIN) == BTN_ON; 

    if (raw != b->raw) {
        b->raw = raw;
        b->lastPressedAtMs = now;
        return true;
    }

    if (now - b->lastPressedAtMs < BTN_DEBOUNCE) {
        return true;
    }

    if (raw != b->isPressed) {
        b->isPressed = raw;
        if (b->isPressed) {
            pressCounter++;
            logPressCount();
        }
    }

    return b->isPressed;
}
#endif

void app_main() {
    init();

    #if TASK > 2
    static Button btn = {};
    #endif

    while(1) {
        #if TASK == 1
        if (lastPressCounter != pressCounter) {
            logPressCount();
            lastPressCounter = pressCounter;
        }
        #endif

        #if TASK > 1
        uint64_t nowMs = esp_timer_get_time() / 1000;

        #if TASK == 2
        if (btnPressedFlag) {
            if (nowMs - lastPressedAtMs >= BTN_DEBOUNCE) {
                lastPressedAtMs = nowMs;
                pressCounter++;
                logPressCount();
            }
            btnPressedFlag = false;
        }
        #endif

        #if TASK > 2 && TASK < 4
        if (btnPressedFlag) {
            isBtnActive = true;
            btnPressedFlag = false;
        }
        if (isBtnActive) {
            isBtnActive = handleButtonUpdate(&btn, nowMs);
        }
        #endif

        #if TASK == 4
        handleButtonUpdate(&btn, nowMs);
        #endif

        #endif

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}