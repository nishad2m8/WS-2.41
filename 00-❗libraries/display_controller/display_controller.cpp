#include "display_controller.h"

static const char *TAG = "display_controller";
static SemaphoreHandle_t lvgl_mux = NULL;

// Define the LCD initialization commands
const sh8601_lcd_init_cmd_t DisplayController::lcd_init_cmds[] = {
    {0xFE, (uint8_t []){0x20}, 1, 0},	
    {0x26, (uint8_t []){0x0A}, 1, 0}, 
    {0x24, (uint8_t []){0x80}, 1, 0}, 

    {0xFE, (uint8_t []){0x00}, 1, 0},    
    {0x3A, (uint8_t []){0x55}, 1, 0},      
    {0xC2, (uint8_t []){0x00}, 1, 10},  
    {0x35, (uint8_t []){0x00}, 0, 0}, 
    {0x51, (uint8_t []){0x00}, 1, 10},
    {0x11, (uint8_t []){0x00}, 0, 80},  
    {0x2A, (uint8_t []){0x00,0x10,0x01,0xD1}, 4, 0},
    {0x2B, (uint8_t []){0x00,0x00,0x02,0x57}, 4, 0},
    {0x29, (uint8_t []){0x00}, 0, 10},
#if (AMOLED_Rotate == Rotate_90)
    {0x36, (uint8_t []){0x30}, 1, 0},     
#endif
    {0x51, (uint8_t []){0xFF}, 1, 0}, // Write Display Brightness MAX_VAL=0XFF
};

// Constructor
DisplayController::DisplayController() : _panel_handle(nullptr), _io_handle(nullptr), _tp(nullptr), _tick_timer(nullptr), _disp(nullptr) {
}

// Destructor
DisplayController::~DisplayController() {
    // Clean up and free resources
    if (_tick_timer) {
        esp_timer_stop(_tick_timer);
        esp_timer_delete(_tick_timer);
    }
    
    if (_panel_handle) {
        esp_lcd_panel_del(_panel_handle);
    }
    
    if (_tp) {
        esp_lcd_touch_del(_tp);
    }
    
    if (lvgl_mux) {
        vSemaphoreDelete(lvgl_mux);
    }
}

// Initialize the display and LVGL
void DisplayController::init() {
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

    // Initialize SPI bus
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(
        EXAMPLE_PIN_NUM_LCD_PCLK,
        EXAMPLE_PIN_NUM_LCD_DATA0,
        EXAMPLE_PIN_NUM_LCD_DATA1,
        EXAMPLE_PIN_NUM_LCD_DATA2,
        EXAMPLE_PIN_NUM_LCD_DATA3,
        EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * LCD_BIT_PER_PIXEL / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Install panel IO
    ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = SH8601_PANEL_IO_QSPI_CONFIG(
        EXAMPLE_PIN_NUM_LCD_CS,
        notify_lvgl_flush_ready,
        &_disp_drv);
    
    sh8601_vendor_config_t vendor_config = {
        .init_cmds = lcd_init_cmds,
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &_io_handle));

    // Install SH8601 panel driver
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    
    ESP_LOGI(TAG, "Install SH8601 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(_io_handle, &panel_config, &_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(_panel_handle, true));

#if EXAMPLE_USE_TOUCH
    // Initialize I2C bus for touch
    ESP_LOGI(TAG, "Initialize I2C bus");
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
        .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {.clk_speed = 300 * 1000},
    };
    
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));
    
    // Configure touch panel
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle));

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_V_RES-1,
        .y_max = EXAMPLE_LCD_H_RES-1,
        .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST,
        .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
#if (AMOLED_Rotate == Rotate_90)
            .swap_xy = 1,
            .mirror_x = 0,
            .mirror_y = 1,
#else
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
#endif
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &_tp));
#endif

    // Initialize LVGL library
    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    
    // Allocate draw buffers used by LVGL
    lv_color_t *buf1 = (lv_color_t*)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = (lv_color_t*)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    
    // Initialize LVGL draw buffers
    lv_disp_draw_buf_init(&_disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_BUF_HEIGHT);

    // Register display driver to LVGL
    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&_disp_drv);
    _disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    _disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    _disp_drv.flush_cb = lvgl_flush_cb;
    _disp_drv.rounder_cb = lvgl_rounder_cb;
    _disp_drv.drv_update_cb = lvgl_update_cb;
    _disp_drv.draw_buf = &_disp_buf;
    _disp_drv.user_data = _panel_handle;
    _disp = lv_disp_drv_register(&_disp_drv);

    // Install LVGL tick timer
    ESP_LOGI(TAG, "Install LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

#if EXAMPLE_USE_TOUCH
    // Register touch input device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = _disp;
    indev_drv.read_cb = lvgl_touch_cb;
    indev_drv.user_data = _tp;
    lv_indev_drv_register(&indev_drv);
#endif

    // Create mutex for LVGL
    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);
    
    // Create LVGL handler task
    xTaskCreate(lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);
    
    // Create UI
    if (lvgl_lock(-1)) {
        create_ui();
        lvgl_unlock();
    }
}

// Create a simple UI
void DisplayController::create_ui() {
    // Default empty UI - override this in your application
    // Left empty instead of adding LVGL demo as requested
}

// Static LVGL callback functions
bool DisplayController::notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

void DisplayController::lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
#if (AMOLED_Rotate == Rotate_90)
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1 + 16;
    const int offsety2 = area->y2 + 16;
#else
    const int offsetx1 = area->x1 + 16;
    const int offsetx2 = area->x2 + 16;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
#endif
    // Copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

void DisplayController::lvgl_update_cb(lv_disp_drv_t *drv) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

    switch (drv->rotated) {
        case LV_DISP_ROT_NONE:
            // Rotate LCD display
            esp_lcd_panel_swap_xy(panel_handle, false);
            esp_lcd_panel_mirror(panel_handle, true, false);
            break;
        case LV_DISP_ROT_90:
            // Rotate LCD display
            esp_lcd_panel_swap_xy(panel_handle, true);
            esp_lcd_panel_mirror(panel_handle, true, true);
            break;
        case LV_DISP_ROT_180:
            // Rotate LCD display
            esp_lcd_panel_swap_xy(panel_handle, false);
            esp_lcd_panel_mirror(panel_handle, false, true);
            break;
        case LV_DISP_ROT_270:
            // Rotate LCD display
            esp_lcd_panel_swap_xy(panel_handle, true);
            esp_lcd_panel_mirror(panel_handle, false, false);
            break;
    }
}

void DisplayController::lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area) {
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;
    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    // Round the start of coordinate down to the nearest 2M number
    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;
    // Round the end of coordinate up to the nearest 2N+1 number
    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

void DisplayController::lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
#if EXAMPLE_USE_TOUCH
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)drv->user_data;
    assert(tp);

    uint16_t tp_x;
    uint16_t tp_y;
    uint8_t tp_cnt = 0;
    
    // Read data from touch controller into memory
    esp_lcd_touch_read_data(tp);
    
    // Read data from touch controller
    bool tp_pressed = esp_lcd_touch_get_coordinates(tp, &tp_x, &tp_y, NULL, &tp_cnt, 1);
    if (tp_pressed && tp_cnt > 0) {
        data->point.x = tp_x;
        data->point.y = tp_y;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGD(TAG, "Touch position: %d,%d", tp_x, tp_y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
#endif
}

void DisplayController::increase_lvgl_tick(void *arg) {
    // Tell LVGL how many milliseconds has elapsed
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool DisplayController::lvgl_lock(int timeout_ms) {
    assert(lvgl_mux && "DisplayController::init must be called first");
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

void DisplayController::lvgl_unlock() {
    assert(lvgl_mux && "DisplayController::init must be called first");
    xSemaphoreGive(lvgl_mux);
}

void DisplayController::lvgl_port_task(void *arg) {
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    
    while (1) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            lvgl_unlock();
        }
        
        if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
        }
        
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}