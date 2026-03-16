# B19 — Text Input

A textarea with an on-screen keyboard. Tapping the textarea reveals the keyboard; typed text is echoed above.

## What it demonstrates

- Creating a textarea with `lv_textarea_create()`
- Pairing a keyboard with `lv_keyboard_set_textarea()`
- Handling focus/defocus events to show/hide keyboard
- Reading textarea content with `lv_textarea_get_text()`
- Placeholder text and one-line mode

## Hardware

- Board: KIT_PSE84_AI / KIT_PSE84_EVAL_EPC2
- Sensors: None

## How to use

1. Copy `main_example.c` to `proj_cm55/app/` replacing the existing file
2. Build and flash: `make build -j && make program`
3. Tap the text field to open the keyboard, type text, press Enter to close
