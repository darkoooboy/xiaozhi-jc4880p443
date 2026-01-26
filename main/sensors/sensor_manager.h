#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "sensor_data.h"
#include <driver/i2c_master.h>
#include <mutex>

class SensorManager {
public:
    static SensorManager& GetInstance();

    // 统一 Start 接口，不带参数（内部自动通过 Board 获取总线）
    void Start();
    
    // 获取最新数据包
    EnvironmentData GetEnvironmentData();
    
    // 供调试使用的串口打印
    void PrintCurrentData();

private:
    SensorManager() = default;
    
    EnvironmentData env_data_ = {0};
    std::mutex data_mutex_;

    // 静态任务函数声明
    static void SensorTask(void* arg);
};

#endif