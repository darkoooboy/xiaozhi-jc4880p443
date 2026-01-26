#include "bme280_sensor.h"
#include <esp_log.h>

#define TAG "BME280"

Bme280Sensor::Bme280Sensor(i2c_master_bus_handle_t bus_handle, uint8_t dev_addr)
    : bus_handle_(bus_handle), dev_addr_(dev_addr) {}

bool Bme280Sensor::Init() {
    if (bus_handle_ == nullptr) return false;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr_,
        .scl_speed_hz = 100000,
    };

    if (i2c_master_bus_add_device(bus_handle_, &dev_cfg, &dev_handle_) != ESP_OK) {
        return false;
    }

    // 探测 Chip ID
    uint8_t chip_id = 0, reg = 0xD0;
    if (i2c_master_transmit_receive(dev_handle_, &reg, 1, &chip_id, 1, 100) != ESP_OK || chip_id != 0x60) {
        ESP_LOGW(TAG, "未找到 BME280 (ID: 0x%02X)", chip_id);
        i2c_master_bus_rm_device(dev_handle_);
        dev_handle_ = nullptr;
        return false;
    }

    // 初始化序列：先写 0xF2 (湿度)，再写 0xF4 (模式)
    uint8_t h_cmd[] = {0xF2, 0x01}; 
    i2c_master_transmit(dev_handle_, h_cmd, 2, 100);
    uint8_t m_cmd[] = {0xF4, 0x27}; 
    i2c_master_transmit(dev_handle_, m_cmd, 2, 100);

    ReadCalibration();
    ESP_LOGI(TAG, "BME280 初始化成功");
    return true;
}

void Bme280Sensor::ReadCalibration() {
    uint8_t buf[26], reg = 0x88;
    i2c_master_transmit_receive(dev_handle_, &reg, 1, buf, 26, 100);
    calib_.T1 = (buf[1] << 8) | buf[0];
    calib_.T2 = (buf[3] << 8) | buf[2];
    calib_.T3 = (buf[5] << 8) | buf[4];
    calib_.P1 = (buf[7] << 8) | buf[6];
    calib_.P2 = (buf[9] << 8) | buf[8];
    calib_.P3 = (buf[11] << 8) | buf[10];
    calib_.P4 = (buf[13] << 8) | buf[12];
    calib_.P5 = (buf[15] << 8) | buf[14];
    calib_.P6 = (buf[17] << 8) | buf[16];
    calib_.P7 = (buf[19] << 8) | buf[18];
    calib_.P8 = (buf[21] << 8) | buf[20];
    calib_.P9 = (buf[23] << 8) | buf[22];

    reg = 0xA1;
    i2c_master_transmit_receive(dev_handle_, &reg, 1, &calib_.H1, 1, 100);
    reg = 0xE1;
    uint8_t h_buf[7];
    i2c_master_transmit_receive(dev_handle_, &reg, 1, h_buf, 7, 100);
    calib_.H2 = (h_buf[1] << 8) | h_buf[0];
    calib_.H3 = h_buf[2];
    calib_.H4 = (h_buf[3] << 4) | (h_buf[4] & 0x0F);
    calib_.H5 = (h_buf[5] << 4) | (h_buf[4] >> 4);
    calib_.H6 = h_buf[6];
}

float Bme280Sensor::CompensateT(int32_t adc_T) {
    float v1 = ((float)adc_T / 16384.0 - (float)calib_.T1 / 1024.0) * (float)calib_.T2;
    float v2 = (((float)adc_T / 131072.0 - (float)calib_.T1 / 8192.0) * ((float)adc_T / 131072.0 - (float)calib_.T1 / 8192.0)) * (float)calib_.T3;
    calib_.t_fine = (int32_t)(v1 + v2);
    return (v1 + v2) / 5120.0;
}

float Bme280Sensor::CompensateP(int32_t adc_P) {
    float v1 = ((float)calib_.t_fine / 2.0) - 64000.0;
    float v2 = v1 * v1 * (float)calib_.P6 / 32768.0;
    v2 = v2 + v1 * (float)calib_.P5 * 2.0;
    v2 = (v2 / 4.0) + ((float)calib_.P4 * 65536.0);
    v1 = ((float)calib_.P3 * v1 * v1 / 524288.0 + (float)calib_.P2 * v1) / 524288.0;
    v1 = (1.0 + v1 / 32768.0) * (float)calib_.P1;
    if (v1 == 0) return 0;
    float p = 1048576.0 - (float)adc_P;
    p = (p - (v2 / 4096.0)) * 6250.0 / v1;
    v1 = (float)calib_.P9 * p * p / 2147483648.0;
    v2 = p * (float)calib_.P8 / 32768.0;
    return (p + (v1 + v2 + (float)calib_.P7) / 16.0) / 100.0;
}

float Bme280Sensor::CompensateH(int32_t adc_H) {
    float h = (float)calib_.t_fine - 76800.0;
    h = (adc_H - ((float)calib_.H4 * 64.0 + (float)calib_.H5 / 16384.0 * h)) * ((float)calib_.H2 / 65536.0 * (1.0 + (float)calib_.H6 / 67108864.0 * h * (1.0 + (float)calib_.H3 / 67108864.0 * h)));
    h = h * (1.0 - (float)calib_.H1 * h / 524288.0);
    if (h > 100.0) h = 100.0; else if (h < 0.0) h = 0.0;
    return h;
}

bool Bme280Sensor::ReadData(float &t, float &h, float &p) {
    if (dev_handle_ == nullptr) return false;
    uint8_t reg = 0xF7, raw[8];
    if (i2c_master_transmit_receive(dev_handle_, &reg, 1, raw, 8, 100) == ESP_OK) {
        int32_t p_raw = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
        int32_t t_raw = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);
        int32_t h_raw = (raw[6] << 8) | raw[7];
        t = CompensateT(t_raw);
        p = CompensateP(p_raw);
        h = CompensateH(h_raw);
        return true;
    }
    return false;
}