#include "motor.h"
#include "gpio.h"





/**
 * @brief 设置电机引脚状态
 * @param portF 电机正转引脚所在的GPIO端口
 * @param pinF 电机正转引脚编号
 * @param stateF 电机正转引脚状态
 * @param portB 电机反转引脚所在的GPIO端口
 * @param pinB 电机反转引脚编号
 * @param stateB 电机反转引脚状态
 * @param portS 电机使能引脚所在的GPIO端口
 * @param pinS 电机使能引脚编号
 * @param stateS 电机使能引脚状态
 * @retval None
*/
static void SetMotorPins(GPIO_TypeDef *portF, uint16_t pinF, GPIO_PinState stateF,
                         GPIO_TypeDef *portB, uint16_t pinB, GPIO_PinState stateB,
                         GPIO_TypeDef *portS, uint16_t pinS, GPIO_PinState stateS)
{
    HAL_GPIO_WritePin(portF, pinF, stateF);
    HAL_GPIO_WritePin(portB, pinB, stateB);
    HAL_GPIO_WritePin(portS, pinS, stateS);
}

/**
 * @brief 设置电机方向
 * @param motor 电机结构体指针
 * @param direction 电机方向
 * @retval None
*/
static void SetMotorDirection(Motor_t *motor, MotorDirection_e direction)
{
    motor->direction = direction;
}

/**
 * @brief 出牌电机正转
 * @param motor 电机结构体指针
 * @retval None
*/
void outMotorForward(Motor_t *motor)
{
    if (motor->id == OUTMOTOR)
    {
        SetMotorDirection(motor, MOTOR_FORWARD);
        SetMotorPins(outMotorFi_GPIO_Port, outMotorFi_Pin, GPIO_PIN_SET,
                     outMotorBi_GPIO_Port, outMotorBi_Pin, GPIO_PIN_RESET,
                     outSdb628Enable_GPIO_Port, outSdb628Enable_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 出牌电机反转
 * @param motor 电机结构体指针
 * @retval None
*/
void outMotorBackward(Motor_t *motor)
{
    if (motor->id == OUTMOTOR)
    {
        SetMotorDirection(motor, MOTOR_REVERSE);
        SetMotorPins(outMotorFi_GPIO_Port, outMotorFi_Pin, GPIO_PIN_RESET,
                     outMotorBi_GPIO_Port, outMotorBi_Pin, GPIO_PIN_SET,
                     outSdb628Enable_GPIO_Port, outSdb628Enable_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 出牌电机停止
 * @param motor 电机结构体指针
 * @retval None
*/
void outMotorStop(Motor_t *motor)
{
    SetMotorDirection(motor, MOTOR_STOP);
    SetMotorPins(outMotorFi_GPIO_Port, outMotorFi_Pin, GPIO_PIN_RESET,
                 outMotorBi_GPIO_Port, outMotorBi_Pin, GPIO_PIN_RESET,
                 outSdb628Enable_GPIO_Port, outSdb628Enable_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 旋转电机正转（）顺时针
 * @param motor 电机结构体指针
 * @retval None
*/
void rotateMotorForward(Motor_t *motor)
{   
    if (motor->id == ROTATEMOTOR)
    {
        SetMotorDirection(motor, MOTOR_FORWARD);
        SetMotorPins(rotateMotorFi_GPIO_Port, rotateMotorFi_Pin, GPIO_PIN_SET,
                     rotateMotorBi_GPIO_Port, rotateMotorBi_Pin, GPIO_PIN_RESET,
                     rotateSdb628Enable_GPIO_Port, rotateSdb628Enable_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 旋转电机反转
 * @param motor 电机结构体指针
 * @retval None
*/
void rotateMotorBackward(Motor_t *motor)
{
    if (motor->id == ROTATEMOTOR)
    {
        SetMotorDirection(motor, MOTOR_REVERSE);
        SetMotorPins(rotateMotorFi_GPIO_Port, rotateMotorFi_Pin, GPIO_PIN_RESET,
                     rotateMotorBi_GPIO_Port, rotateMotorBi_Pin, GPIO_PIN_SET,
                     rotateSdb628Enable_GPIO_Port, rotateSdb628Enable_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 旋转电机停止
 * @param motor 电机结构体指针
 * @retval None
*/
void rotateMotorStop(Motor_t *motor)
{
    SetMotorDirection(motor, MOTOR_STOP);   
    SetMotorPins(rotateMotorFi_GPIO_Port, rotateMotorFi_Pin, GPIO_PIN_RESET,
                 rotateMotorBi_GPIO_Port, rotateMotorBi_Pin, GPIO_PIN_RESET,
                 rotateSdb628Enable_GPIO_Port, rotateSdb628Enable_Pin, GPIO_PIN_RESET);
}



