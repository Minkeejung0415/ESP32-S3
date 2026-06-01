#include "dio_input.h"

#include "driver/gpio.h"

static int s_gpio = -1;
static uint32_t s_edge_count;

void dio_input_init(int gpio_num)
{
    s_gpio = gpio_num;
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << gpio_num,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
}

int16_t dio_input_read_channel(void)
{
    if (s_gpio < 0) {
        return 0;
    }
    int level = gpio_get_level(s_gpio);
    return (int16_t)((level & 1) | ((s_edge_count & 0x7FFF) << 1));
}
