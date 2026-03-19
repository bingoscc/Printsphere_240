/**
 * @file printer_state.h
 * @brief 打印机状态数据结构
 *
 * 包含打印机所有关键状态信息:
 * - 打印进度 (百分比)
 * - 温度 (喷嘴、床基)
 * - 剩余时间
 * - 作业名称
 * - 层信息
 */

#ifndef PRINTER_STATE_H
#define PRINTER_STATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 打印机状态枚举
 */
typedef enum {
    PRINTER_IDLE,       // 空闲
    PRINTER_PRINTING,   // 打印中
    PRINTER_PAUSED,     // 已暂停
    PRINTER_COMPLETED,  // 完成
    PRINTER_ERROR,      // 错误
    PRINTER_OFFLINE,    // 离线
    PRINTER_UNKNOWN     // 未知
} printer_status_t;

/**
 * @brief 打印机状态结构体
 */
typedef struct {
    printer_status_t status;          // 当前状态
    char job_name[128];                // 作业名称
    char gcode_file[256];              // G代码文件名
    float progress;                    // 打印进度 (0-100%)
    int remaining_time;                // 剩余时间 (秒)
    float nozzle_temp;                 // 喷嘴温度 (摄氏度)
    float nozzle_target_temp;          // 喷嘴目标温度
    float bed_temp;                    // 床基温度
    float bed_target_temp;             // 床基目标温度
    float layer_speed;                 // 层打印速度
    int current_layer;                 // 当前层
    int total_layers;                  // 总层数
    float humidity;                    // 环境湿度
    float AMS_humidity;               // AMS humidity
    bool is_ams_inserted;             // AMS是否插入
    int AMS_current_slot;             // AMS当前槽位
    float consumption;                // 耗材消耗 (米)
    char start_time[32];             // 开始时间
    char end_time[32];               // 结束时间
    bool is_chamber_temp_available;   // 腔体温度是否可用
    float chamber_temp;               // 腔体温度
} printer_state_t;

/**
 * @brief 初始化打印机状态
 * @param state 状态结构体指针
 */
void printer_state_init(printer_state_t *state);

/**
 * @brief 重置打印机状态
 * @param state 状态结构体指针
 */
void printer_state_reset(printer_state_t *state);

/**
 * @brief 状态枚举转字符串
 * @param status 状态枚举
 * @return 状态字符串
 */
const char* printer_status_to_string(printer_status_t status);

#ifdef __cplusplus
}
#endif

#endif // PRINTER_STATE_H
