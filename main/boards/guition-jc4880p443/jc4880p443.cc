#include "wifi_board.h"
#include "audio/codecs/es8311_audio_codec.h"
#include "application.h"
#include "display/lcd_display.h"
#include "button.h"
#include "config.h"
#include "led/single_led.h"
#include "sensors/sensor_manager.h" 

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_st7701.h"
#include "esp_lcd_touch_gt911.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>

#define TAG "jc4880p443"

// LCD 厂商初始化序列 (保持原样)
static const st7701_lcd_init_cmd_t lcd_cmd[] = {
    {0xFF, (uint8_t []){0x77,0x01,0x00,0x00,0x13},5,0},
    {0xEF, (uint8_t []){0x08}, 1, 0},
    {0xFF, (uint8_t []){0x77,0x01,0x00,0x00,0x10},5,0},
    {0xC0, (uint8_t []){0x63, 0x00}, 2, 0},
    {0xC1, (uint8_t []){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t []){0x10, 0x08}, 2, 0},
    {0xCC, (uint8_t []){0x10}, 1, 0},
    {0xB0, (uint8_t []){0x80, 0x09, 0x53, 0x0C, 0xD0, 0x07, 0x0C, 0x09, 0x09, 0x28, 0x06, 0xD4, 0x13, 0x69, 0x2B, 0x71}, 16, 0},
    {0xB1, (uint8_t []){0x80, 0x94, 0x5A, 0x10, 0xD3, 0x06, 0x0A, 0x08, 0x08, 0x25, 0x03, 0xD3, 0x12, 0x66, 0x6A, 0x0D}, 16, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t []){0x5D}, 1, 0},
    {0xB1, (uint8_t []){0x58}, 1, 0},
    {0xB2, (uint8_t []){0x87}, 1, 0},
    {0xB3, (uint8_t []){0x80}, 1, 0},
    {0xB5, (uint8_t []){0x4E}, 1, 0},
    {0xB7, (uint8_t []){0x85}, 1, 0},
    {0xB8, (uint8_t []){0x21}, 1, 0},
    {0xB9, (uint8_t []){0x10, 0x1F}, 2, 0},
    {0xBB, (uint8_t []){0x03}, 1,0},
    {0xBC, (uint8_t []){0x00}, 1,0},
    {0xC1, (uint8_t []){0x78}, 1, 0},
    {0xC2, (uint8_t []){0x78}, 1, 0},
    {0xD0, (uint8_t []){0x88}, 1, 0},
    {0xE0, (uint8_t []){0x00, 0x3A, 0x02}, 3, 0},
    {0xE1, (uint8_t []){0x04, 0xA0, 0x00, 0xA0, 0x05,0xA0, 0x00, 0xA0, 0x00, 0x40, 0x40}, 11, 0},
    {0xE2, (uint8_t []){0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00}, 13, 0},
    {0xE3, (uint8_t []){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t []){0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0}, 16, 0},
    {0xE6, (uint8_t []){0x00, 0x00, 0x33, 0x33}, 4, 0},
    {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t []){0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0}, 16, 0},
    {0xEB, (uint8_t []){0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00}, 7, 0},
    {0xEC, (uint8_t []){0x08, 0x01}, 2, 0},
    {0xED, (uint8_t []){0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B}, 16, 0},
    {0xEF, (uint8_t []){0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},
    {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x11, (uint8_t []){0x00}, 1, 120},
    {0x29, (uint8_t []){0x00}, 1, 20},
};

class jc4880p443 : public WifiBoard {
private:
    i2c_master_bus_handle_t codec_i2c_bus_;
    Button boot_button_; 
    LcdDisplay *display__;
    esp_lcd_touch_handle_t touch_handle_ = nullptr;
    bool test_mode_active_ = false;

    void InitializeCodecI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_1,
            .sda_io_num = AUDIO_CODEC_I2C_SDA_PIN,
            .scl_io_num = AUDIO_CODEC_I2C_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = { .enable_internal_pullup = 1 },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
    }

    void InitializeGT911() {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << LCD_TOUCH_RST) | (1ULL << LCD_TOUCH_INT),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
        gpio_set_level((gpio_num_t)LCD_TOUCH_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(50)); 
        gpio_set_level((gpio_num_t)LCD_TOUCH_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(100));

        const esp_lcd_touch_config_t tp_cfg = {
            .x_max = LCD_H_RES,
            .y_max = LCD_V_RES,
            .rst_gpio_num = (gpio_num_t)GPIO_NUM_NC, 
            .int_gpio_num = (gpio_num_t)GPIO_NUM_NC, 
            .levels = { .reset = 0, .interrupt = 0 },
            .flags = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 },
        };
        esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
        tp_io_config.dev_addr = 0x5D;
        tp_io_config.scl_speed_hz = 100000; 
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        if (esp_lcd_new_panel_io_i2c(codec_i2c_bus_, &tp_io_config, &tp_io_handle) == ESP_OK) {
            if (esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle_) == ESP_OK) {
                ESP_LOGI(TAG, "GT911 touch driver installed");
            }
        }
    }

    void InitializeLCD() {
        bsp_enable_dsi_phy_power();
        esp_lcd_panel_io_handle_t io = NULL;
        esp_lcd_panel_handle_t disp_panel = NULL;
        esp_lcd_dsi_bus_handle_t mipi_dsi_bus = NULL;
        esp_lcd_dsi_bus_config_t bus_config = { .bus_id = 0, .num_data_lanes = LCD_MIPI_DSI_LANE_NUM, .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT, .lane_bit_rate_mbps = 500 };
        esp_lcd_new_dsi_bus(&bus_config, &mipi_dsi_bus);
        esp_lcd_dbi_io_config_t dbi_config = { .virtual_channel = 0, .lcd_cmd_bits = 8, .lcd_param_bits = 8 };
        esp_lcd_new_panel_io_dbi(mipi_dsi_bus, &dbi_config, &io);
        esp_lcd_dpi_panel_config_t dpi_config = { .virtual_channel = 0, .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT, .dpi_clock_freq_mhz = 34, .pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565, .num_fbs = 2, .video_timing = { .h_size = 480, .v_size = 800, .hsync_pulse_width = 12, .hsync_back_porch = 42, .hsync_front_porch = 42, .vsync_pulse_width = 2, .vsync_back_porch = 8, .vsync_front_porch = 166 }, .flags={ .use_dma2d = true } };
        st7701_vendor_config_t v_cfg = { .init_cmds = lcd_cmd, .init_cmds_size = sizeof(lcd_cmd)/sizeof(st7701_lcd_init_cmd_t), .mipi_config = { .dsi_bus = mipi_dsi_bus, .dpi_config = &dpi_config }, .flags = { .use_mipi_interface = 1 } };
        const esp_lcd_panel_dev_config_t l_cfg = { .reset_gpio_num = PIN_NUM_LCD_RST, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, .bits_per_pixel = 16, .vendor_config = &v_cfg };
        esp_lcd_new_panel_st7701(io, &l_cfg, &disp_panel);
        esp_lcd_panel_reset(disp_panel); esp_lcd_panel_init(disp_panel);
        display__ = new MipiLcdDisplay(io, disp_panel, LCD_H_RES, LCD_V_RES, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY, touch_handle_);
    }

    static esp_err_t bsp_enable_dsi_phy_power(void) {
        #if MIPI_DSI_PHY_PWR_LDO_CHAN > 0
            static esp_ldo_channel_handle_t p_chan = NULL;
            esp_ldo_channel_config_t l_cfg = { .chan_id = MIPI_DSI_PHY_PWR_LDO_CHAN, .voltage_mv = MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV };
            esp_ldo_acquire_channel(&l_cfg, &p_chan);
        #endif
        return ESP_OK;
    }

    void RunTouchTest() {
        if (touch_handle_ == nullptr) return;
        uint16_t x[1], y[1], s[1]; uint8_t c;
        while (test_mode_active_) {
            esp_lcd_touch_read_data(touch_handle_);
            if (esp_lcd_touch_get_coordinates(touch_handle_, x, y, s, &c, 1) && c > 0) {
                printf("Touch: %d, %d\n", x[0], y[0]);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    void InitializeButtons() {
        boot_button_.OnLongPress([this]() {
            test_mode_active_ = !test_mode_active_;
            if (test_mode_active_) {
                ESP_LOGI(TAG, "Test mode activated - Printing sensor data:");
                // 调用 SensorManager 打印环境传感器信息
                SensorManager::GetInstance().PrintCurrentData();
                
                // 启动触摸测试任务
                xTaskCreate([](void* p){ 
                    ((jc4880p443*)p)->RunTouchTest(); 
                    vTaskDelete(NULL); 
                }, "tch_test", 4096, this, 5, NULL);
            }
        });
    }

public:
    jc4880p443() : boot_button_(BOOT_BUTTON_GPIO) {  
        InitializeCodecI2c(); 
        InitializeGT911(); 
        InitializeLCD(); 
        InitializeButtons();
        GetBacklight()->RestoreBrightness();
    }

    virtual ~jc4880p443() = default;

    // 实现基类接口，暴露 I2C 总线给 SensorManager
    virtual i2c_master_bus_handle_t GetI2CBus() override {
        return codec_i2c_bus_;
    }

    virtual Led* GetLed() override { 
        static SingleLed led(BUILTIN_LED_GPIO); 
        return &led; 
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec codec(codec_i2c_bus_, I2C_NUM_1, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE, AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN, AUDIO_CODEC_PA_PIN, AUDIO_CODEC_ES8311_ADDR);
        return &codec;
    }

    virtual Display* GetDisplay() override { 
        return display__; 
    }

    virtual Backlight* GetBacklight() override { 
        static PwmBacklight bl(PIN_NUM_BK_LIGHT, DISPLAY_BACKLIGHT_OUTPUT_INVERT); 
        return &bl; 
    }
};

DECLARE_BOARD(jc4880p443);