#!/usr/bin/env bash
# 主机侧框架测试：用假 HAL 在 PC 上验证 driver/app 逻辑与时序。
# 用法：bash tests/run_tests.sh   （在工程根目录执行）
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

gcc -std=c11 -Wall -Wextra -Werror \
    -I "$ROOT/include" -I "$ROOT/tests" \
    "$ROOT/tests/test_framework.c" \
    "$ROOT/src/drivers/key/button.c" \
    "$ROOT/src/drivers/led/led.c" \
    "$ROOT/src/drivers/led/bicolor_led.c" \
    "$ROOT/src/app/app.c" \
    -o "$OUT/tests"

"$OUT/tests"
