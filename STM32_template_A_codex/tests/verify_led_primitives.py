from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

FORBIDDEN_DRIVER_TOKENS = {
    "include/drivers/led/led.h": [
        "LED_MODE_BLINK",
        "led_mode_t",
        "led_set_blink",
        "led_tick",
        "on_ms",
        "off_ms",
        "next_toggle_ms",
    ],
    "src/drivers/led/led.c": [
        "led_set_blink",
        "led_tick",
        "next_toggle_ms",
    ],
    "include/drivers/led/bicolor_led.h": [
        "bicolor_led_blink",
    ],
    "src/drivers/led/bicolor_led.c": [
        "bicolor_led_blink",
    ],
}


def test_driver_layer_only_exposes_led_primitives() -> list[str]:
    failures = []
    for relative_path, tokens in FORBIDDEN_DRIVER_TOKENS.items():
        text = (ROOT / relative_path).read_text(encoding="utf-8")
        for token in tokens:
            if token in text:
                failures.append(f"{relative_path}: remove driver-layer token {token!r}")

    return failures


def test_app_owns_timing_words() -> list[str]:
    app_text = (ROOT / "src/app/app.c").read_text(encoding="utf-8")
    failures = []

    if "now_ms" not in app_text:
        failures.append("src/app/app.c: app must receive now_ms for business-owned timing")
    if "last_toggle_ms" not in app_text and "last_change_ms" not in app_text:
        failures.append("src/app/app.c: app must store its own blink timing state")

    return failures


def main() -> int:
    failures = []
    failures.extend(test_driver_layer_only_exposes_led_primitives())
    failures.extend(test_app_owns_timing_words())

    if failures:
        print("\n".join(failures))
        return 1

    print("LED primitive architecture checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
