#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#include "state_machine.h"

typedef enum {
    SENSOR_EVT_POWER_ON = SM_EVENT_BASE,
    SENSOR_EVT_RESET,
    SENSOR_EVT_READ,
    SENSOR_EVT_CALIBRATE,
    SENSOR_EVT_FAULT,
    SENSOR_EVT_RECOVER
} sensor_event_t;

typedef struct {
    sm_machine_t sm;
    const char  *name;
    float        temperature;
    float        humidity;
    bool         init_should_fail;
    int          retry_count;
    int          max_retries;
    bool         recovered;
} sensor_driver_t;

void sensor_driver_init(sensor_driver_t *dev, const char *name, bool fail_first_try);
void sensor_driver_deinit(sensor_driver_t *dev);
sm_error_t sensor_driver_start(sensor_driver_t *dev);
sm_error_t sensor_driver_read(sensor_driver_t *dev, float *temp, float *hum);
sm_error_t sensor_driver_trigger_fault(sensor_driver_t *dev);
sm_error_t sensor_driver_try_recover(sensor_driver_t *dev);
void sensor_driver_dump(sensor_driver_t *dev);

#endif
