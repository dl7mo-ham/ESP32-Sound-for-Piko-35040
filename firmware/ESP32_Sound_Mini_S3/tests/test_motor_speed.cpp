/*
 * tests/test_motor_speed.cpp  -  Soundmodul Mini (projektlokal)
 *
 * Host-Tests fuer die Arduino-freien Helfer des virtuellen Gaswerts.
 * Start: sh firmware/ESP32_Sound_Mini_S3/tests/run_tests.sh
 */

#ifdef RC_COMMON_HOST_TEST

#include <unity.h>
#include "motor_speed_control.h"

void setUp() {}
void tearDown() {}

void test_motor_speed_step_clamp() {
    TEST_ASSERT_EQUAL_UINT8(MOTOR_SPEED_STEP_DEFAULT, motorSpeedStepClamp(20));
    TEST_ASSERT_EQUAL_UINT8(MOTOR_SPEED_STEP_MIN, motorSpeedStepClamp(-5));
    TEST_ASSERT_EQUAL_UINT8(MOTOR_SPEED_STEP_MIN, motorSpeedStepClamp(0));
    TEST_ASSERT_EQUAL_UINT8(MOTOR_SPEED_STEP_MAX, motorSpeedStepClamp(99));
}

void test_motor_speed_advance() {
    TEST_ASSERT_EQUAL_UINT8(20, motorSpeedAdvance(0, +1, 20));
    TEST_ASSERT_EQUAL_UINT8(40, motorSpeedAdvance(20, +1, 20));
    TEST_ASSERT_EQUAL_UINT8(0, motorSpeedAdvance(10, -1, 20));
    TEST_ASSERT_EQUAL_UINT8(100, motorSpeedAdvance(90, +1, 20));
    TEST_ASSERT_EQUAL_UINT8(80, motorSpeedAdvance(100, -1, 20));
}

void test_motor_speed_playback_base() {
    TEST_ASSERT_EQUAL_FLOAT(100.0f, motorSpeedPlaybackBase(0, 100, 200));
    TEST_ASSERT_EQUAL_FLOAT(150.0f, motorSpeedPlaybackBase(50, 100, 200));
    TEST_ASSERT_EQUAL_FLOAT(200.0f, motorSpeedPlaybackBase(100, 100, 200));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, motorSpeedPlaybackBase(100, 100, 100));
}

void test_motor_speed_ramp_direction() {
    // "+" erhoeht, "-" verringert - auch wenn die Rampe noch hinterherhinkt.
    TEST_ASSERT_EQUAL_FLOAT(120.0f, motorSpeedRampStep(100.0f, 120.0f, 20.0f));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, motorSpeedRampStep(120.0f, 100.0f, 20.0f));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, motorSpeedRampStep(80.0f, 100.0f, 20.0f));  // kein Ueberschwingen
    TEST_ASSERT_EQUAL_FLOAT(80.0f, motorSpeedRampStep(100.0f, 80.0f, 20.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f,  motorSpeedRampStep(5.0f, 0.0f, 20.0f));     // kein Negativwert
    TEST_ASSERT_EQUAL_FLOAT(0.0f,  motorSpeedRampStep(0.0f, 0.0f, 20.0f));
}

void test_motor_speed_reset_on_stop() {
    TEST_ASSERT_EQUAL_UINT8(0,  motorSpeedResetOnStop(50, true, false));  // Aus -> Reset
    TEST_ASSERT_EQUAL_UINT8(50, motorSpeedResetOnStop(50, false, true));  // Ein -> behalten
    TEST_ASSERT_EQUAL_UINT8(50, motorSpeedResetOnStop(50, true, true));   // Lauf -> behalten
    TEST_ASSERT_EQUAL_UINT8(50, motorSpeedResetOnStop(50, false, false)); // Aus -> nichts zu tun
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_motor_speed_step_clamp);
    RUN_TEST(test_motor_speed_advance);
    RUN_TEST(test_motor_speed_playback_base);
    RUN_TEST(test_motor_speed_ramp_direction);
    RUN_TEST(test_motor_speed_reset_on_stop);
    return UNITY_END();
}

#endif // RC_COMMON_HOST_TEST
