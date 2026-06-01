#pragma once

#include <stdint.h>

void dio_input_init(int gpio_num);
int16_t dio_input_read_channel(void);
