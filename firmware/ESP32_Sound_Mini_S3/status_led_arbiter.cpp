/*
 * status_led_arbiter.cpp  -  Soundmodul Mini (siehe status_led_arbiter.h)
 */

#include "status_led_arbiter.h"
#include "flame_led.h"

void ledArbiterUpdate(void (*statusUpdate)()) {
    if (flameIsOn()) {
        flameUpdate();          // Flamme hat Vorrang, Status-LED wird übersprungen
    } else if (statusUpdate) {
        statusUpdate();
    }
}
