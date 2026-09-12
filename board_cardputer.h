#ifndef BOARD_CARDPUTER_H
#define BOARD_CARDPUTER_H

#include "esp_err.h"

esp_err_t board_cardputer_init(void);
float board_cardputer_get_battery_voltage(void);

#endif // BOARD_CARDPUTER_H
