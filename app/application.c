#include <application.h>

// LED instance
twr_led_t led;

// Button instance
twr_button_t button;

twr_tmp112_t tmp112;
float tmp112_celsius = NAN;
bool tmp112_ok = false;

twr_lis2dh12_t lis2dh12;
twr_lis2dh12_result_g_t lis2dh12_g = { NAN, NAN, NAN };
bool lis2dh12_ok = false;

twr_sht30_t sht30;
float sht30_humidity = NAN;
bool sht30_ok = false;

twr_opt3001_t opt3001;
float opt3001_lux = NAN;
bool opt3001_ok = false;

bool m24c04_ok = false;

uint8_t si7210_data = 0xff;
bool si7210_ok = false;

twr_gfx_t *gfx;
twr_led_t led_green;
twr_led_t led_red;

// Button event callback
void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    // Log button event
    twr_log_info("APP: Button event: %i", event);
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    tmp112_celsius = NAN;

    tmp112_ok = twr_tmp112_get_temperature_celsius(self, &tmp112_celsius);

    twr_log_debug("TMP112: %d, %.1f", tmp112_ok, tmp112_celsius);
}

void lis2dh12_event_handler(twr_lis2dh12_t *self, twr_lis2dh12_event_t event, void *event_param)
{
    lis2dh12_g.x_axis = NAN;
    lis2dh12_g.y_axis = NAN;
    lis2dh12_g.z_axis = NAN;

    lis2dh12_ok = twr_lis2dh12_get_result_g(self, &lis2dh12_g);

    twr_log_debug("LIS2DH12: %d, [%.2f,%.2f,%.2f]", lis2dh12_ok, lis2dh12_g.x_axis, lis2dh12_g.y_axis, lis2dh12_g.z_axis);
}

void sht30_event_handler(twr_sht30_t *self, twr_sht30_event_t event, void *event_param)
{
    sht30_humidity = NAN;

    sht30_ok = twr_sht30_get_humidity_percentage(self, &sht30_humidity);

    twr_log_debug("SHT30: %d, %.1f", sht30_ok, sht30_humidity);
}

void opt3001_event_handler(twr_opt3001_t *self, twr_opt3001_event_t event, void *event_param)
{
    opt3001_lux = NAN;

    opt3001_ok = twr_opt3001_get_illuminance_lux(self, &opt3001_lux);

    twr_log_debug("OPT3001: %d, %.1f", opt3001_ok, opt3001_lux);
}

void m24c04_test_task(void *param)
{
    static uint8_t buffer[16];
    static twr_i2c_memory_transfer_t transfere = {
        .device_address = 0x51,
        .memory_address = 0x00,
        .buffer = buffer,
        .length = sizeof(buffer)
    };

    m24c04_ok = twr_i2c_memory_read(TWR_I2C_I2C_1W, &transfere);

    twr_log_debug("M24C04: %d", m24c04_ok);

    twr_scheduler_plan_current_from_now(1000);
}

void si7210_test_task(void *param)
{
    si7210_data = 0xFF;

    si7210_ok = twr_i2c_memory_read_8b(TWR_I2C_I2C_1W, 0x32, 0xC0, &si7210_data);

    twr_log_debug("SI7210: %d %02X", si7210_ok, si7210_data);

    twr_scheduler_plan_current_from_now(1000);
}

// Application initialization function which is called once after boot
void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, 0);
    twr_led_pulse(&led, 2000);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, 0);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    twr_tmp112_init(&tmp112, TWR_I2C_I2C_1W, 0x48);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, 1000);

    twr_lis2dh12_init(&lis2dh12, TWR_I2C_I2C_1W, 0x19);
    twr_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);
    twr_lis2dh12_set_update_interval(&lis2dh12, 1000);

    twr_sht30_init(&sht30, TWR_I2C_I2C_1W, 0x45);
    twr_sht30_set_event_handler(&sht30, sht30_event_handler, NULL);
    twr_sht30_set_update_interval(&sht30, 1000);

    twr_opt3001_init(&opt3001, TWR_I2C_I2C_1W, 0x44);
    twr_opt3001_set_event_handler(&opt3001, opt3001_event_handler, NULL);
    twr_opt3001_set_update_interval(&opt3001, 1000);

    // M24C04 - EERPROM
    twr_scheduler_register(m24c04_test_task, NULL, 0);

    // Si7210 -  Hall Sensor
    twr_scheduler_register(si7210_test_task, NULL, 0);

    twr_module_lcd_init();
    gfx = twr_module_lcd_get_gfx();

    twr_led_init_virtual(&led_green, TWR_MODULE_LCD_LED_GREEN, twr_module_lcd_get_led_driver(), 1);
    twr_led_init_virtual(&led_red, TWR_MODULE_LCD_LED_RED, twr_module_lcd_get_led_driver(), 1);
}

// Application task function (optional) which is called peridically if scheduled
void application_task(void)
{
    if (!twr_gfx_display_is_ready(gfx))
    {
        twr_scheduler_plan_current_now();
        return;
    }

    twr_system_pll_enable();

    twr_gfx_clear(gfx);

    bool ok = !tmp112_ok && lis2dh12_ok && sht30_ok && opt3001_ok && m24c04_ok && si7210_ok;

    if (ok) {
        twr_log_info("OK");
    } else {
        twr_log_error("ERROR");
    }

    twr_led_pulse(ok ? &led_green : &led_red, 100);

    twr_gfx_set_font(gfx, &twr_font_ubuntu_15);

    twr_gfx_draw_string(gfx, 5, 5 + (0 * 15), "TMP112", 1);
    twr_gfx_draw_string(gfx, 70, 5 + (0 * 15), tmp112_ok ? "ok": "err", 1);
    twr_gfx_printf(gfx, 100, 5 + (0 * 15), 1, "%.1f", tmp112_celsius);

    twr_gfx_draw_string(gfx, 5, 5 + (1 * 15), "LIS2DH12", 1);
    twr_gfx_draw_string(gfx, 70, 5 + (1 * 15), lis2dh12_ok ? "ok": "err", 1);
    twr_gfx_printf(gfx, 100, 5 + (1 * 15), 1, "%.1f", lis2dh12_g.z_axis);

    twr_gfx_draw_string(gfx, 5, 5 + (2 * 15), "SHT30", 1);
    twr_gfx_draw_string(gfx, 70, 5 + (2 * 15), sht30_ok ? "ok": "err", 1);
    twr_gfx_printf(gfx, 100, 5 + (2 * 15), 1, "%.1f", sht30_humidity);

    twr_gfx_draw_string(gfx, 5, 5 + (3 * 15), "OPT3001", 1);
    twr_gfx_draw_string(gfx, 70, 5 + (3 * 15), opt3001_ok ? "ok": "err", 1);
    twr_gfx_printf(gfx, 100, 5 + (3 * 15), 1, "%.0f", opt3001_lux);

    twr_gfx_draw_string(gfx, 5, 5 + (4 * 15), "M24C04", 1);
    twr_gfx_draw_string(gfx, 70, 5 + (4 * 15), m24c04_ok ? "ok": "err", 1);

    twr_gfx_draw_string(gfx, 5, 5 + (5 * 15), "SI7210", 1);
    twr_gfx_draw_string(gfx, 70, 5 + (5 * 15), si7210_ok ? "ok": "err", 1);

    twr_gfx_set_font(gfx, &twr_font_ubuntu_24);
    twr_gfx_draw_string(gfx, 5, 100, ok ? "OK OK OK": "ERROR", 1);

    twr_gfx_update(gfx);

    twr_scheduler_plan_current_from_now(1000);

    twr_system_pll_disable();
}
