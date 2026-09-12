After cloning the official solar_os repository, place your BSP files and configure the project using the exact directory structure below:
1. File Placement
Copy the files from your BSP repository into the cloned solar_os repository like this:
solar_os/
├── platformio.ini                              <-- Open and append the [env:m5stack_cardputer] block here
└── src/
    └── boards/
        └── m5stack_cardputer/                  <-- Copy your whole board folder here
            ├── m5stack_cardputer.toml
            ├── board_cardputer.h
            ├── board_cardputer.c
            ├── keyboard_matrix.h
            ├── keyboard_matrix.c
            ├── display_st7789.h
            └── display_st7789.c

2. Register Board in src/main.c
Open src/main.c in the cloned repo and add the initialization call to app_main():
#include "boards/m5stack_cardputer/board_cardputer.h"

void app_main(void)
{
    // Add this line at the start of app_main:
    board_cardputer_init();
    
    // ... rest of SolarOS startup code
}

3. Build the Binary
Open your terminal in the root solar_os/ directory and run:
pio run -e m5stack_cardputer

4. Flash / Deploy
 * Via M5Launcher (SD Card): Take .pio/build/m5stack_cardputer/firmware.bin, rename it to SolarOS_Cardputer_v11.bin, put it in the /firmware/ folder on your microSD card, and launch it from M5Launcher on the device.
 * Via Direct USB Flash: Connect the Cardputer to USB and run:
   pio run -e m5stack_cardputer -t upload

