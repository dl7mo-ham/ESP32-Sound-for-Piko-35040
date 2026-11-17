/*
 * tests/test_app_roles.cpp  -  Soundmodul Mini (projektlokal)
 *
 * Host-Tests fuer die Projektrolle Reed (100) und die Migration des alten
 * Werts 5. Die RC_Common-Testdateien bleiben dadurch upstream-identisch.
 * Start: sh firmware/ESP32_Sound_Mini_S3/tests/run_tests.sh
 */

#ifdef RC_COMMON_HOST_TEST

#include <unity.h>
#include "reed_roles.h"
#include "src/RC_Common/uart_roles.h"

void setUp() {}
void tearDown() {}

void test_app_role_names_and_options() {
    TEST_ASSERT_EQUAL_STRING("Aus;CRSF;S.Port;GPS;Raw;Reed", appRoleOptions());
    TEST_ASSERT_EQUAL_STRING("Reed", appRoleName(APP_UROLE_REED));
    TEST_ASSERT_EQUAL_STRING("Reed", appRoleName(APP_UROLE_REED_LEGACY));
    TEST_ASSERT_EQUAL_STRING("GPS", appRoleName(UROLE_GPS));
    TEST_ASSERT_EQUAL_STRING("Aus", appRoleName(101));
}

void test_app_role_clamp_and_migration() {
    TEST_ASSERT_EQUAL_UINT8(APP_UROLE_REED, appRoleClamp(APP_UROLE_REED));
    TEST_ASSERT_EQUAL_UINT8(APP_UROLE_REED, appRoleClamp(5));    // v0.14
    TEST_ASSERT_EQUAL_UINT8(UROLE_OFF, appRoleClamp(-1));
    TEST_ASSERT_EQUAL_UINT8(UROLE_OFF, appRoleClamp(99));        // ausserhalb 0..4/100
    TEST_ASSERT_EQUAL_UINT8(UROLE_RAW, appRoleClamp(UROLE_RAW));
    // RC_Common selbst kennt 100 nicht - die Projektrolle bleibt lokal.
    TEST_ASSERT_EQUAL_UINT8(UROLE_OFF, uartRoleClamp(APP_UROLE_REED));
}

void test_app_role_baud_and_ui_index() {
    TEST_ASSERT_EQUAL_UINT32(0, appRoleDefaultBaud(APP_UROLE_REED));
    TEST_ASSERT_EQUAL_UINT32(420000, appRoleDefaultBaud(UROLE_CRSF));
    TEST_ASSERT_EQUAL_UINT32(UART_BAUD_AUTO, appRoleDefaultBaud(UROLE_GPS));
    TEST_ASSERT_EQUAL_UINT8(APP_UROLE_REED_INDEX, appRoleUiIndex(APP_UROLE_REED));
    TEST_ASSERT_EQUAL_UINT8(UROLE_GPS, appRoleUiIndex(UROLE_GPS));
    // UI-Index 5 (Reed) wird beim Setzen wieder auf 100 abgebildet.
    TEST_ASSERT_EQUAL_UINT8(APP_UROLE_REED, appRoleClamp(appRoleUiIndex(APP_UROLE_REED)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_app_role_names_and_options);
    RUN_TEST(test_app_role_clamp_and_migration);
    RUN_TEST(test_app_role_baud_and_ui_index);
    return UNITY_END();
}

#endif // RC_COMMON_HOST_TEST
