# BUGS

Known issues in the SolarOS Cardputer BSP, with fixes described but not yet applied. Written against solar_os upstream HEAD as of 2026-09-12 (commit e61a1dc).

## 1. SD card cannot mount (blocking)

The display and the SD card both claim SPI2_HOST with different pin maps. The ST7789 init configures the bus with MOSI on GPIO 21, then the SD mount calls the SDSPI host on the same controller with MOSI on GPIO 14. Two problems stack here. The second bus-initialize call on an already-claimed controller returns an invalid-state error, so the mount fails. And even if it did not, a single SPI controller has one MOSI signal; two different pins on one bus is not something the hardware can express.

Fix: on Cardputer hardware the TF slot shares the TFT bus. Drop the separate SD MOSI pin, initialize the bus exactly once, and add the SD card as a second device on that bus with its own CS on GPIO 12. When the port moves onto upstream abstractions, route both devices through the routed SPI bus manager instead of raw bus init, which makes this class of collision structurally impossible. The board manifest's sdcard section should also be corrected to reflect the shared MOSI.

## 2. Keyboard code does not compile against upstream HEAD (blocking)

The keyboard scanner includes solar_events.h and posts through the solar event queue. That interface no longer exists upstream. Input now goes through the input service in src/services/solar_os_input.h: sources register by class, modifiers use the canonical USB HID bit definitions, and repeat behavior is owned by the service rather than the driver.

Fix: translate the matrix decode onto the input service. Register a keyboard-class input source and emit input events with the shift modifier set from the HID shift bits. Doing this also removes issue 5 below, since autorepeat comes from the service for free.

## 3. Keyboard manifest does not match the code (documentation)

The board manifest declares a 4-row by 14-column matrix. The scanner walks 8 rows by 7 columns, which is what the 74HC138 actually provides: three address lines select among eight outputs. Both spellings multiply to 56 keys, but only the 8x7 shape matches the hardware. Correct the manifest.

## 4. README file placement does not match upstream layout (documentation)

The README instructs copying BSP files into src/boards/m5stack_cardputer/ and adding a board init call to app_main(). Upstream has no src/boards directory, and main.c does not take per-board init calls. Boards are wired through manifests under boards/manifests/ plus capability files under src/board/. Rewrite the placement instructions around the manifest system, and pin the exact upstream commit the BSP was written against while editing this file.

## 5. Fn key is detected but does nothing (minor)

The Fn key position is read into the modifier flags but no Fn keymap layer exists, so nothing consumes it. Add a third keymap table and select it when Fn is held. Worth doing after the input-service port (issue 2), since the service carries modifiers cleanly.

## 6. No key repeat (minor)

Edge detection posts one event per press; holding a key produces a single character. Fine for a shell prompt, painful for text entry. The preferred fix is the upstream input service's built-in repeat, reached via issue 2. A driver-level fallback is possible in the scan loop, but do not build both.

## 7. Display flush blocks the RTOS (minor)

Full-frame flush uses polling transmit: about 65 KB pushed busy-wait, roughly 13 ms of frozen scheduler per frame at 40 MHz. Acceptable for a shell, wasteful for anything animated. Move to queued transactions backed by DMA. Separately, the backlight is hardcoded on while the manifest promises PWM at 128; wire it to the LEDC peripheral if brightness control matters. The battery read uses the legacy ADC driver, deprecated in newer IDF, harmless today, worth a swap when convenient.

## Worth keeping as-is

The ST7789 driver fills a genuine gap: upstream ships st7796, ili9341, st7305, SSD1306, PCD8544, epaper and VGA drivers and no ST7789 anywhere. The pin map, panel offsets and init sequence match the Cardputer panel. The matrix decode and keymaps are correct. The battery divider math is right. These are the parts worth contributing back first.
