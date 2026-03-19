/**
 * @file printer_state.cpp
 * @brief 打印机状态实现
 *
 * 提供打印机状态结构的初始化、重置和状态转换功能
 */

#include "printer_state.h"
#include <string.h>

/**
 * @brief 初始化打印机状态
 *
 * 将所有字段设置为默认值，状态设为离线
 */
void printer_state_init(printer_state_t *state) {
    memset(state, 0, sizeof(printer_state_t));
    state->status = PRINTER_OFFLINE;  // 默认离线状态
    state->progress = 0;              // 进度0%
    state->remaining_time = -1;       // 剩余时间未知
    state->nozzle_temp = 0;          // 喷嘴温度
    state->nozzle_target_temp = 0;    // 喷嘴目标温度
    state->bed_temp = 0;             // 床基温度
    state->bed_target_temp = 0;      // 床基目标温度
    state->current_layer = 0;         // 当前层
    state->total_layers = 0;         // 总层数
}

/**
 * @brief 重置打印机状态
 *
 * 保留当前状态值，只重置其他字段
 * 用于刷新状态时保持状态不变
 */
void printer_state_reset(printer_state_t *state) {
    printer_status_t old_status = state->status;
    printer_state_init(state);
    state->status = old_status;
}

/**
 * @brief 状态枚举转中文字符串
 */
const char* printer_status_to_string(printer_status_t status) {
    switch (status) {
        case PRINTER_IDLE:      return "空闲";
        case PRINTER_PRINTING:  return "打印中";
        case PRINTER_PAUSED:    return "已暂停";
        case PRINTER_COMPLETED: return "已完成";
        case PRINTER_ERROR:     return "错误";
        case PRINTER_OFFLINE:   return "离线";
        default:                return "未知";
    }
}
