#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <time.h>

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
    time_t last_update;
} EnvironmentData;

#endif