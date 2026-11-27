#!/bin/sh
# Projektlokale Host-Tests (Reed-Rolle 100). RC_Common/tests/run_tests.sh bleibt unberuehrt.
set -e
T=$(cd "$(dirname "$0")" && pwd)
S=$(dirname "$T")
R="$S/src/RC_Common"
U="${UNITY_DIR:-${TMPDIR:-/tmp}/rc_common_unity}"
[ -f "$U/src/unity.c" ] || git -c advice.detachedHead=false clone -q --depth 1 -b v2.6.1 https://github.com/ThrowTheSwitch/Unity "$U"
B="${BUILD_DIR:-${TMPDIR:-/tmp}/app_roles_build}"
mkdir -p "$B"
CXX="${CXX:-g++}"
FLAGS="-DRC_COMMON_HOST_TEST -std=c++17 -Wall -Wno-unused-function -I$S -I$R -I$U/src"
"${CC:-gcc}" -I"$U/src" "$U/src/unity.c" -c -o "$B/unity.o"
"$CXX" $FLAGS "$T/test_app_roles.cpp" "$S/reed_roles.cpp" "$R/uart_roles.cpp" "$B/unity.o" -o "$B/test_app_roles"
"$CXX" $FLAGS "$T/test_ir_code.cpp" "$S/ir_code.cpp" "$B/unity.o" -o "$B/test_ir_code"
"$CXX" $FLAGS "$T/test_motor_speed.cpp" "$B/unity.o" -o "$B/test_motor_speed"
"$B/test_app_roles"
"$B/test_ir_code"
"$B/test_motor_speed"
