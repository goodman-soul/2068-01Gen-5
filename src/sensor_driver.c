#include "sensor_driver.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static sm_error_t idle_on_entry(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)sm;
    (void)event;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] -> IDLE (powered off)\n", dev->name);
    dev->temperature = 0.0f;
    dev->humidity = 0.0f;
    return SM_OK;
}

static sm_error_t idle_on_event(sm_machine_t *sm, sm_event_t event, void *data)
{
    sensor_driver_t *dev = (sensor_driver_t *)data;
    switch (event) {
    case SENSOR_EVT_POWER_ON:
        printf("[SENSOR:%s] IDLE: power on request, transitioning to INIT\n", dev->name);
        return sm_transition(sm, SM_STATE_INIT);
    default:
        printf("[SENSOR:%s] IDLE: ignoring event %d (not powered)\n", dev->name, event);
        return SM_OK;
    }
}

static sm_error_t init_on_entry(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)event;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] -> INIT (starting initialization...)\n", dev->name);
    printf("[SENSOR:%s] INIT: performing hardware self-check...\n", dev->name);

    if (dev->init_should_fail && dev->retry_count == 0) {
        printf("[SENSOR:%s] INIT: first attempt FAIL (simulated fault)\n", dev->name);
        dev->retry_count++;
        return sm_transition(sm, SM_STATE_ERROR);
    }

    printf("[SENSOR:%s] INIT: self-check OK, calibration passed\n", dev->name);
    dev->temperature = 25.0f;
    dev->humidity = 50.0f;
    dev->retry_count = 0;
    return sm_transition(sm, SM_STATE_READY);
}

static sm_error_t init_on_event(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)sm;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] INIT: busy initializing, ignoring event %d\n", dev->name, event);
    return SM_OK;
}

static sm_error_t init_on_exit(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)sm; (void)event;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] <- INIT leaving\n", dev->name);
    return SM_OK;
}

static sm_error_t ready_on_entry(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)event; (void)sm;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] -> READY (operational) temp=%.1fC hum=%.1f%%\n",
           dev->name, dev->temperature, dev->humidity);
    return SM_OK;
}

static sm_error_t ready_on_event(sm_machine_t *sm, sm_event_t event, void *data)
{
    sensor_driver_t *dev = (sensor_driver_t *)data;
    switch (event) {
    case SENSOR_EVT_READ:
        dev->temperature += (float)(rand() % 10 - 5) * 0.1f;
        dev->humidity += (float)(rand() % 10 - 5) * 0.2f;
        printf("[SENSOR:%s] READY: sample taken temp=%.1fC hum=%.1f%%\n",
               dev->name, dev->temperature, dev->humidity);
        return SM_OK;
    case SENSOR_EVT_FAULT:
        printf("[SENSOR:%s] READY: fault detected, moving to ERROR\n", dev->name);
        return sm_transition(sm, SM_STATE_ERROR);
    case SENSOR_EVT_RESET:
        printf("[SENSOR:%s] READY: reset requested, moving to IDLE\n", dev->name);
        return sm_transition(sm, SM_STATE_IDLE);
    default:
        printf("[SENSOR:%s] READY: event %d not handled\n", dev->name, event);
        return SM_OK;
    }
}

static sm_error_t error_on_entry(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)event; (void)sm;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] -> ERROR (fault state, retry_count=%d)\n", dev->name, dev->retry_count);
    return SM_OK;
}

static sm_error_t error_on_event(sm_machine_t *sm, sm_event_t event, void *data)
{
    sensor_driver_t *dev = (sensor_driver_t *)data;
    switch (event) {
    case SENSOR_EVT_RESET:
        printf("[SENSOR:%s] ERROR: hard reset -> IDLE\n", dev->name);
        dev->retry_count = 0;
        dev->init_should_fail = false;
        return sm_transition(sm, SM_STATE_IDLE);
    case SENSOR_EVT_RECOVER:
        printf("[SENSOR:%s] ERROR: recovery attempt -> RECOVER\n", dev->name);
        return sm_transition(sm, SM_STATE_RECOVER);
    default:
        printf("[SENSOR:%s] ERROR: ignoring event %d while in error\n", dev->name, event);
        return SM_OK;
    }
}

static sm_error_t recover_on_entry(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)event;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] -> RECOVER (attempting recovery...)\n", dev->name);
    dev->retry_count++;

    if (dev->retry_count >= dev->max_retries) {
        printf("[SENSOR:%s] RECOVER: max retries (%d) reached, recovery FAILED\n",
               dev->name, dev->max_retries);
        dev->recovered = false;
        return sm_transition(sm, SM_STATE_ERROR);
    }

    printf("[SENSOR:%s] RECOVER: retry %d/%d - reinitializing sensor...\n",
           dev->name, dev->retry_count, dev->max_retries);
    dev->temperature = 25.0f;
    dev->humidity = 50.0f;
    dev->recovered = true;
    dev->init_should_fail = false;

    printf("[SENSOR:%s] RECOVER: success! moving to READY\n", dev->name);
    return sm_transition(sm, SM_STATE_READY);
}

static sm_error_t recover_on_event(sm_machine_t *sm, sm_event_t event, void *data)
{
    (void)sm;
    sensor_driver_t *dev = (sensor_driver_t *)data;
    printf("[SENSOR:%s] RECOVER: busy recovering, ignoring event %d\n", dev->name, event);
    return SM_OK;
}

static void sensor_setup_states(sensor_driver_t *dev)
{
    sm_state_info_t info;

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_IDLE;
    strcpy(info.name, "IDLE");
    info.on_entry = idle_on_entry;
    info.on_event = idle_on_event;
    sm_register_state(&dev->sm, &info);

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_INIT;
    strcpy(info.name, "INIT");
    info.on_entry = init_on_entry;
    info.on_event = init_on_event;
    info.on_exit = init_on_exit;
    sm_register_state(&dev->sm, &info);

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_READY;
    strcpy(info.name, "READY");
    info.on_entry = ready_on_entry;
    info.on_event = ready_on_event;
    sm_register_state(&dev->sm, &info);

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_ERROR;
    strcpy(info.name, "ERROR");
    info.on_entry = error_on_entry;
    info.on_event = error_on_event;
    sm_register_state(&dev->sm, &info);

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_RECOVER;
    strcpy(info.name, "RECOVER");
    info.on_entry = recover_on_entry;
    info.on_event = recover_on_event;
    sm_register_state(&dev->sm, &info);

    sm_add_transition(&dev->sm, SM_STATE_IDLE, SM_STATE_INIT);

    sm_add_transition(&dev->sm, SM_STATE_INIT, SM_STATE_READY);
    sm_add_transition(&dev->sm, SM_STATE_INIT, SM_STATE_ERROR);
    sm_add_transition(&dev->sm, SM_STATE_INIT, SM_STATE_IDLE);

    sm_add_transition(&dev->sm, SM_STATE_READY, SM_STATE_ERROR);
    sm_add_transition(&dev->sm, SM_STATE_READY, SM_STATE_IDLE);

    sm_add_transition(&dev->sm, SM_STATE_ERROR, SM_STATE_IDLE);
    sm_add_transition(&dev->sm, SM_STATE_ERROR, SM_STATE_RECOVER);

    sm_add_transition(&dev->sm, SM_STATE_RECOVER, SM_STATE_READY);
    sm_add_transition(&dev->sm, SM_STATE_RECOVER, SM_STATE_ERROR);
}

void sensor_driver_init(sensor_driver_t *dev, const char *name, bool fail_first_try)
{
    memset(dev, 0, sizeof(sensor_driver_t));
    dev->name = name;
    dev->init_should_fail = fail_first_try;
    dev->retry_count = 0;
    dev->max_retries = 3;
    dev->recovered = false;
    sm_init(&dev->sm, dev);
    sensor_setup_states(dev);
    sm_state_info_t *idle = NULL;
    for (size_t i = 0; i < dev->sm.num_states; i++) {
        if (dev->sm.states[i].id == SM_STATE_IDLE) {
            idle = &dev->sm.states[i];
            break;
        }
    }
    if (idle && idle->on_entry) {
        idle->on_entry(&dev->sm, SM_EVENT_ENTRY, dev);
    }
}

void sensor_driver_deinit(sensor_driver_t *dev)
{
    printf("[SENSOR:%s] deinitializing driver\n", dev->name);
    memset(dev, 0, sizeof(sensor_driver_t));
}

sm_error_t sensor_driver_start(sensor_driver_t *dev)
{
    return sm_dispatch(&dev->sm, SENSOR_EVT_POWER_ON, dev);
}

sm_error_t sensor_driver_read(sensor_driver_t *dev, float *temp, float *hum)
{
    sm_error_t err = sm_dispatch(&dev->sm, SENSOR_EVT_READ, dev);
    if (err == SM_OK && temp && hum) {
        *temp = dev->temperature;
        *hum = dev->humidity;
    }
    return err;
}

sm_error_t sensor_driver_trigger_fault(sensor_driver_t *dev)
{
    return sm_dispatch(&dev->sm, SENSOR_EVT_FAULT, dev);
}

sm_error_t sensor_driver_try_recover(sensor_driver_t *dev)
{
    return sm_dispatch(&dev->sm, SENSOR_EVT_RECOVER, dev);
}

void sensor_driver_dump(sensor_driver_t *dev)
{
    printf("=== Sensor Driver: %s ===\n", dev->name);
    sm_print_state(&dev->sm);
    printf("  temperature: %.2f C\n", dev->temperature);
    printf("  humidity:    %.2f %%\n", dev->humidity);
    printf("  retry_count: %d/%d\n", dev->retry_count, dev->max_retries);
    printf("  recovered:   %s\n", dev->recovered ? "yes" : "no");
    printf("==========================\n");
}
