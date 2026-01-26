#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// ============================================
// 音频配置
// ============================================
#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 16000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_13
#define AUDIO_I2S_GPIO_WS GPIO_NUM_10
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_12
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_48
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_9

// ============================================
// I2C配置
// ============================================
#define I2C_SDA_PIN        GPIO_NUM_7
#define I2C_SCL_PIN        GPIO_NUM_8

// ============================================
// 音频编解码器配置
// ============================================
#define AUDIO_CODEC_PA_PIN       GPIO_NUM_11
#define AUDIO_CODEC_I2C_SDA_PIN  I2C_SDA_PIN
#define AUDIO_CODEC_I2C_SCL_PIN  I2C_SCL_PIN
#define AUDIO_CODEC_ES8311_ADDR  ES8311_CODEC_DEFAULT_ADDR
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false

// ============================================
// BME280环境传感器配置（使用官方组件）
// ============================================
#ifndef CONFIG_BME280_ENABLE
#define CONFIG_BME280_ENABLE 1  // 启用BME280传感器
#endif

// BME280 I2C地址（根据SDO引脚选择）
#define BME280_I2C_ADDRESS  0x76  // SDO接地
// #define BME280_I2C_ADDRESS  0x77  // SDO接VCC

// 传感器参数配置（这些宏在jc4880p443.cc中被使用）
// 使用官方BME280组件的枚举值
#ifndef BME280_TEMP_SAMPLING
#define BME280_TEMP_SAMPLING  BME280_SAMPLING_X2
#endif

#ifndef BME280_PRES_SAMPLING
#define BME280_PRES_SAMPLING  BME280_SAMPLING_X2
#endif

#ifndef BME280_HUM_SAMPLING
#define BME280_HUM_SAMPLING   BME280_SAMPLING_X2
#endif

#ifndef BME280_IIR_FILTER
#define BME280_IIR_FILTER     BME280_FILTER_X4
#endif

#ifndef BME280_STANDBY_TIME
#define BME280_STANDBY_TIME   BME280_STANDBY_MS_1000
#endif

// ============================================
// 测试模式配置
// ============================================
#ifndef CONFIG_TEST_MODE_ENABLE
#define CONFIG_TEST_MODE_ENABLE 1  // 启用测试模式
#endif

// ============================================
// 通用IO配置
// ============================================
#define BUILTIN_LED_GPIO    GPIO_NUM_26
#define BOOT_BUTTON_GPIO    GPIO_NUM_35

// ============================================
// 显示配置
// ============================================
#define MIPI_DPI_PX_FORMAT  (LCD_COLOR_PIXEL_FORMAT_RGB888)
#define DISPLAY_SWAP_XY false
#define DISPLAY_MIRROR_X false
#define DISPLAY_MIRROR_Y false
#define BACKLIGHT_INVERT false

#define DISPLAY_OFFSET_X  0
#define DISPLAY_OFFSET_Y  0

#define LCD_H_RES          (480)
#define LCD_V_RES          (800)
#define LCD_BIT_PER_PIXEL  (16)
#define PIN_NUM_LCD_RST    GPIO_NUM_5
#define PIN_NUM_BK_LIGHT   GPIO_NUM_23
#define LCD_BK_LIGHT_ON_LEVEL      (1)
#define LCD_BK_LIGHT_OFF_LEVEL     !TEST_LCD_BK_LIGHT_ON_LEVEL

#define DELAY_TIME_MS      (3000)
#define LCD_MIPI_DSI_LANE_NUM (2)
#define BSP_LCD_COLOR_SPACE (ESP_LCD_COLOR_SPACE_RGB)

#define MIPI_DSI_PHY_PWR_LDO_CHAN       (3)
#define MIPI_DSI_PHY_PWR_LDO_VOLTAGE_MV (2500)

// ============================================
// 触摸屏配置
// ============================================
#define LCD_TOUCH_RST      GPIO_NUM_22
#define LCD_TOUCH_INT      GPIO_NUM_21

// 触摸调试配置
#ifndef CONFIG_TOUCH_DEBUG_ENABLE
#define CONFIG_TOUCH_DEBUG_ENABLE 1
#endif

#ifndef CONFIG_TOUCH_DEBUG_INTERVAL_MS
#define CONFIG_TOUCH_DEBUG_INTERVAL_MS 200
#endif

#ifndef CONFIG_TOUCH_MAX_POINTS
#define CONFIG_TOUCH_MAX_POINTS 5
#endif

#endif // _BOARD_CONFIG_H_