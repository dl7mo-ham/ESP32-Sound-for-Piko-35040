/*
 * tests/test_ir_code.cpp  -  Soundmodul Mini (projektlokal)
 *
 * Host-Tests fuer die IR-Code-Darstellung (NVS/JSON-Stabilitaet).
 * Start: sh firmware/ESP32_Sound_Mini_S3/tests/run_tests.sh
 */

#ifdef RC_COMMON_HOST_TEST

#include <unity.h>
#include "ir_code.h"

void setUp() {}
void tearDown() {}

void test_ir_code_valid_and_clamp() {
    IrCode empty = { IR_PROTO_NONE, 0x1234, 0x56 };
    TEST_ASSERT_FALSE(irCodeValid(empty));
    TEST_ASSERT_EQUAL_UINT8(IR_PROTO_NONE, irCodeClampProtocol(-1));
    TEST_ASSERT_EQUAL_UINT8(IR_PROTO_NONE, irCodeClampProtocol(0));
    TEST_ASSERT_EQUAL_UINT8(IR_PROTO_NONE, irCodeClampProtocol(IR_PROTO_COUNT));
    TEST_ASSERT_EQUAL_UINT8(IR_PROTO_NEC,  irCodeClampProtocol(IR_PROTO_NEC));
    TEST_ASSERT_EQUAL_UINT8(IR_PROTO_NEC2, irCodeClampProtocol(IR_PROTO_NEC2));
}

void test_ir_code_equal() {
    IrCode a = { IR_PROTO_NEC, 0x00A4, 0x12 };
    IrCode b = { IR_PROTO_NEC, 0x00A4, 0x12 };
    IrCode proto = { IR_PROTO_NEC2, 0x00A4, 0x12 };
    IrCode addr  = { IR_PROTO_NEC, 0x00A5, 0x12 };
    IrCode cmd   = { IR_PROTO_NEC, 0x00A4, 0x13 };
    TEST_ASSERT_TRUE(irCodeEqual(a, b));
    TEST_ASSERT_FALSE(irCodeEqual(a, proto));
    TEST_ASSERT_FALSE(irCodeEqual(a, addr));
    TEST_ASSERT_FALSE(irCodeEqual(a, cmd));
    // Zwei leere Codes sind gleich - das wird beim Zuordnen ueber
    // irCodeValid() abgefangen.
    IrCode e1 = { IR_PROTO_NONE, 0, 0 };
    IrCode e2 = { IR_PROTO_NONE, 0, 0 };
    TEST_ASSERT_TRUE(irCodeEqual(e1, e2));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ir_code_valid_and_clamp);
    RUN_TEST(test_ir_code_equal);
    return UNITY_END();
}

#endif // RC_COMMON_HOST_TEST
