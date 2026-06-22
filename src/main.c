#include <stdio.h>
#include <string.h>
#include "sensor_driver.h"

static void print_section(const char *title)
{
    printf("\n======================================\n");
    printf("  %s\n", title);
    printf("======================================\n");
}

static void test_illegal_transition(void)
{
    print_section("Test 1: 非法状态跳转测试");
    sm_machine_t sm;
    sm_state_info_t info;

    sm_init(&sm, NULL);

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_IDLE;
    strcpy(info.name, "IDLE");
    sm_register_state(&sm, &info);

    memset(&info, 0, sizeof(info));
    info.id = SM_STATE_READY;
    strcpy(info.name, "READY");
    sm_register_state(&sm, &info);

    sm_add_transition(&sm, SM_STATE_IDLE, SM_STATE_INIT);
    sm_add_transition(&sm, SM_STATE_INIT, SM_STATE_READY);

    printf("当前状态: %s\n", sm_get_state_name(&sm, sm_get_state(&sm)));
    printf("尝试 IDLE -> READY (未注册该跳转)...\n");
    sm_error_t err = sm_transition(&sm, SM_STATE_READY);
    if (err == SM_ERR_TRANSITION) {
        printf("  OK: 非法跳转已阻止，错误码 SM_ERR_TRANSITION (%d)\n", err);
    } else {
        printf("  FAIL: 预期错误但得到 %d\n", err);
    }
    sm_print_state(&sm);
}

static void test_normal_init(void)
{
    print_section("Test 2: 传感器正常启动流程");
    sensor_driver_t dev;
    float t, h;

    sensor_driver_init(&dev, "TempHum-01", false);
    sensor_driver_dump(&dev);

    printf("\n启动传感器...\n");
    sensor_driver_start(&dev);
    sensor_driver_dump(&dev);

    printf("\n读取传感器数据...\n");
    for (int i = 0; i < 3; i++) {
        sensor_driver_read(&dev, &t, &h);
        printf("  采样 #%d: temp=%.2fC  hum=%.2f%%\n", i + 1, t, h);
    }

    sensor_driver_deinit(&dev);
}

static void test_init_failure_and_recover(void)
{
    print_section("Test 3: 初始化失败 + 恢复流程");
    sensor_driver_t dev;

    sensor_driver_init(&dev, "TempHum-02", true);
    sensor_driver_dump(&dev);

    printf("\n启动传感器 (首次会失败)...\n");
    sensor_driver_start(&dev);
    sensor_driver_dump(&dev);

    printf("\n尝试恢复...\n");
    sensor_driver_try_recover(&dev);
    sensor_driver_dump(&dev);

    printf("\n恢复后读取数据...\n");
    float t, h;
    sensor_driver_read(&dev, &t, &h);
    printf("  temp=%.2fC  hum=%.2f%%\n", t, h);

    sensor_driver_deinit(&dev);
}

static void test_runtime_fault(void)
{
    print_section("Test 4: 运行时故障 + 复位");
    sensor_driver_t dev;

    sensor_driver_init(&dev, "TempHum-03", false);
    sensor_driver_start(&dev);
    sensor_driver_dump(&dev);

    printf("\n触发运行时故障...\n");
    sensor_driver_trigger_fault(&dev);
    sensor_driver_dump(&dev);

    printf("\n从 ERROR 状态执行复位...\n");
    sm_dispatch(&dev.sm, SENSOR_EVT_RESET, &dev);
    sensor_driver_dump(&dev);

    printf("\n重新上电启动...\n");
    sensor_driver_start(&dev);
    sensor_driver_dump(&dev);

    sensor_driver_deinit(&dev);
}

int main(void)
{
    printf("设备驱动状态机框架演示\n");

    test_illegal_transition();
    test_normal_init();
    test_init_failure_and_recover();
    test_runtime_fault();

    print_section("全部测试完成");
    return 0;
}
