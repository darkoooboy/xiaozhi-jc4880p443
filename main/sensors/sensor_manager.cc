#include "sensor_manager.h"
#include "board.h"
#include "bme280_sensor.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "SensorManager"

SensorManager& SensorManager::GetInstance() {
    static SensorManager instance;
    return instance;
}

void SensorManager::Start() {
    // 启动后台任务，所有硬件探测在任务内部完成，避免阻塞主线程
    xTaskCreate(SensorTask, "sensor_task", 4096, nullptr, 5, nullptr);
}

EnvironmentData SensorManager::GetEnvironmentData() {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return env_data_;
}

void SensorManager::PrintCurrentData() {
    auto data = GetEnvironmentData();
    if (data.valid) {
        printf("BME280 >> 温度: %.1f°C, 湿度: %.1f%%, 气压: %.1f hPa\n", 
               data.temperature, data.humidity, data.pressure);
    } else {
        printf("BME280 >> 暂无有效数据 (传感器初始化失败或正在读取)\n");
    }
}

void SensorManager::SensorTask(void* arg) {
    auto& board = Board::GetInstance();
    auto bus = board.GetI2CBus();
    auto& self = SensorManager::GetInstance();

    if (bus == nullptr) {
        ESP_LOGE(TAG, "无法获取 I2C 总线，传感器任务终止");
        vTaskDelete(nullptr);
        return;
    }

    // 静态实例化驱动，确保生命周期贯穿整个任务
    static Bme280Sensor bme(bus, 0x76);
    if (!bme.Init()) {
        ESP_LOGW(TAG, "未检测到 BME280 硬件");
        vTaskDelete(nullptr);
        return;
    }

    ESP_LOGI(TAG, "BME280 初始化成功，开始采集...");

    while (true) {
        float t, h, p;
        if (bme.ReadData(t, h, p)) {
            // 更新数据池并加锁
            std::lock_guard<std::mutex> lock(self.data_mutex_);
            self.env_data_.temperature = t;
            self.env_data_.humidity = h;
            self.env_data_.pressure = p;
            self.env_data_.valid = true;
            self.env_data_.last_update = time(nullptr);
        } else {
            std::lock_guard<std::mutex> lock(self.data_mutex_);
            self.env_data_.valid = false;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}