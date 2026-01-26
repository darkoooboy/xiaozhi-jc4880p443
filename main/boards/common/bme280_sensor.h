#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include <driver/i2c_master.h>
#include <esp_log.h>

class Bme280Sensor {
public:
    Bme280Sensor(i2c_master_bus_handle_t bus_handle, uint8_t dev_addr);
    bool Init();
    bool ReadData(float &temp, float &hum, float &pres);

private:
    i2c_master_bus_handle_t bus_handle_;
    i2c_master_dev_handle_t dev_handle_ = nullptr;
    uint8_t dev_addr_;

    struct {
        uint16_t T1; int16_t T2, T3;
        uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
        uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
        int32_t t_fine;
    } calib_;

    void ReadCalibration();
    float CompensateT(int32_t adc_T);
    float CompensateP(int32_t adc_P);
    float CompensateH(int32_t adc_H);
};

#endif