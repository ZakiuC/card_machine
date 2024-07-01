#ifndef __MOTOR_H
#define __MOTOR_H
#include "main.h"


typedef enum {
    OUTMOTOR = 0,       // 出牌电机
    ROTATEMOTOR,        // 旋转电机
} MotorId_e;

typedef enum {
    MOTOR_REVERSE = -1,       // 反转
    MOTOR_STOP,               // 停止
    MOTOR_FORWARD,            // 正转
} MotorDirection_e;

// typedef enum{
//     RUNNING_NORMAL,
//     RUNNING_WARNING,
//     RUNNING_ERROR,
// } MotorRuning_e;

typedef struct{
    MotorId_e id;               // 电机ID
    // MotorRuning_e state;
    int16_t current_pos;        // 定位点
    MotorDirection_e direction; // 旋转方向
    uint16_t cards;             // 已发的牌数
    uint32_t totalCards;        // 总共发的牌数
} Motor_t;


void outMotorForward(Motor_t *motor);
void outMotorBackward(Motor_t *motor);
void outMotorStop(Motor_t *motor);
void rotateMotorForward(Motor_t *motor);
void rotateMotorBackward(Motor_t *motor);
void rotateMotorStop(Motor_t *motor);
#endif /* __MOTOR_H */
