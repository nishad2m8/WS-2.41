#ifndef DISPLAY_CONTROLLER_H
#define DISPLAY_CONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lvgl.h"
#include "esp_lcd_sh8601.h"
#include "esp_lcd_touch_ft5x06.h"

// Define rotation constants
#define Rotate_90      0
#define Rotate_NONO    1

// Rotate_90 Landscape screen
// Rotate_NONO Portrait screen
#define AMOLED_Rotate  Rotate_NONO

// LCD Configuration
#define LCD_HOST    SPI2_HOST
#define TOUCH_HOST  I2C_NUM_0
#define LCD_BIT_PER_PIXEL       (16)

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_LCD_CS            (GPIO_NUM_9)
#define EXAMPLE_PIN_NUM_LCD_PCLK          (GPIO_NUM_10) 
#define EXAMPLE_PIN_NUM_LCD_DATA0         (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1         (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2         (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3         (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST           (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT          (-1)

// Touch Configuration
#define EXAMPLE_USE_TOUCH              1 //Without tp ---- Touch off

#if EXAMPLE_USE_TOUCH
#define EXAMPLE_PIN_NUM_TOUCH_SCL         (GPIO_NUM_48)
#define EXAMPLE_PIN_NUM_TOUCH_SDA         (GPIO_NUM_47)
#define EXAMPLE_PIN_NUM_TOUCH_RST         (gpio_num_t)(-1)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (gpio_num_t)(-1)
#endif

// The pixel number in horizontal and vertical
#if (AMOLED_Rotate == Rotate_90)
#define EXAMPLE_LCD_H_RES              600    
#define EXAMPLE_LCD_V_RES              450
#else
#define EXAMPLE_LCD_H_RES              450  
#define EXAMPLE_LCD_V_RES              600
#endif

// LVGL Task Configuration
#define EXAMPLE_LVGL_BUF_HEIGHT        (EXAMPLE_LCD_V_RES/10)
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

// Display Controller Class
class DisplayController {
public:
    DisplayController();
    ~DisplayController();
    
    // Initialize the display and LVGL
    void init();
    
    // Lock LVGL mutex with timeout in ms
    static bool lvgl_lock(int timeout_ms = -1);
    
    // Unlock LVGL mutex
    static void lvgl_unlock();
    
    // Get LVGL display handle
    lv_disp_t* get_display() { return _disp; }
    
    // Create a simple UI (can be overridden with your own UI)
    virtual void create_ui();

    private:
    // LVGL task handler
    static void lvgl_port_task(void *arg);
    
    // LVGL callback functions
    static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
    static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
    static void lvgl_update_cb(lv_disp_drv_t *drv);
    static void lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area);
    static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);
    static void increase_lvgl_tick(void *arg);
    
    // LCD initialization commands
    static const sh8601_lcd_init_cmd_t lcd_init_cmds[];
    
    // Display driver and buffer
    lv_disp_drv_t _disp_drv;
    lv_disp_draw_buf_t _disp_buf;
    lv_disp_t* _disp;
    
    // Hardware handles
    esp_lcd_panel_handle_t _panel_handle;
    esp_lcd_panel_io_handle_t _io_handle;
    esp_lcd_touch_handle_t _tp;
    esp_timer_handle_t _tick_timer;
};

#endif // DISPLAY_CONTROLLER_H