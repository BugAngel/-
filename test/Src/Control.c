/**
  ******************************************************************************
  * @file    control.h
  * @author  WangRong
  * @version V1.0
  * @date    2016/1/13
  * @brief   control
  * @note    YAW ��ת��С����ת����
  ******************************************************************************
  */
#include "stm32f4xx_hal.h"
#include "pid.h"
#include "Control.h"
#include "Motor.h"

extern PidTypeDef RFMotorPID;
extern PidTypeDef LBMotorPID;
extern PidTypeDef LFMotorPID;
extern PidTypeDef RBMotorPID;
extern PidTypeDef YawPID;    //�Ƕ�PID

extern int16_t MotorSpeed1;//��ǰ  ��ת��������ת����
extern int16_t MotorSpeed2;//���  ��ת��������ת����
extern int16_t MotorSpeed3;//��ǰ  ��ת��������ת����
extern int16_t MotorSpeed4;//�Һ�  ��ת��������ת����

extern int32_t UpperSetSpeed;
extern uint8_t MotorFlag;       //С�������ź�
extern int32_t LeftForwardSpeed;//��ǰ�ָı��ٶ�
extern int32_t RightForwardSpeed;//��ǰ�ָı��ٶ�

uint32_t RBtemp = 0; //�Һ���ʱPWM
uint32_t RFtemp = 0; //��ǰ��ʱPWM
uint32_t LBtemp = 0; //�����ʱPWM
uint32_t LFtemp = 0; //��ǰ��ʱPWM

void PID_init(void)
{
    PID_SetParam(&RFMotorPID, 2, 0, -0.4, 0, 0, 0, 0);  //��ǰ����ջ�PID
    PID_SetParam(&LBMotorPID, 2, 0, -0.4, 0, 0, 0, 0);  //������ջ�PID
    PID_SetParam(&LFMotorPID, 2, 0, -0.4, 0, 0, 0, 0);  //��ǰ����ջ�PID
    PID_SetParam(&RBMotorPID, 2, 0, -0.4, 0, 0, 0, 0);  //�Һ����ջ�PID
    PID_SetParam(&YawPID, 2, 0, -0.4, 0, 0, 0, 0);  //�Ƕȱջ�PID

}
/**
  * @brief  PID����ͣ��
  * @param  NONE
  * @retval NONE
  */
void PIDStop(void)
{
    MotorFlag = 0;
    PID_SpeedCalc(&RFMotorPID, MotorSpeed1, 0);
    MotorSpeedRF((int32_t)RFMotorPID.U);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&LBMotorPID, MotorSpeed2, 0);
    MotorSpeedLB((int32_t)LBMotorPID.U);  //�����ת�ٿ���

    PID_SpeedCalc(&LFMotorPID, -MotorSpeed3, 0);
    MotorSpeedLF((int32_t)LFMotorPID.U);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&RBMotorPID, MotorSpeed4, 0);
    MotorSpeedRB((int32_t)RBMotorPID.U);  //�Һ���ת�ٿ���

}
/**
  * @brief  ����ǰ��תʱ���ת��
  * @param  UpperSetSpeed:�趨�������ٶ�
  * @retval NONE
  */
void UpperSpeedControl(int32_t UpperSetSpeed)
{
    PID_SpeedCalc(&RFMotorPID, -MotorSpeed1, UpperSetSpeed);
    RFtemp += (int32_t)RFMotorPID.U;
    MotorSpeedRF(RFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&LBMotorPID, -MotorSpeed2, UpperSetSpeed);
    LBtemp += (int32_t)LBMotorPID.U;
    MotorSpeedLB(LBtemp);  //�����ת�ٿ���

    PID_SpeedCalc(&LFMotorPID, MotorSpeed3, UpperSetSpeed);
    LFtemp += (int32_t)LFMotorPID.U;
    MotorSpeedLF(LFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&RBMotorPID, -MotorSpeed4, UpperSetSpeed);
    RBtemp += (int32_t)RBMotorPID.U;
    MotorSpeedRB(RBtemp);  //�Һ���ת�ٿ���
}

/**
  * @brief  PID����
  * @param
                    @arg  Yaw:��ǰ�Ƕ�
                    @arg SetYaw :�趨�Ƕ�
  * @retval NONE
  */
void Pid_Control(float yaw, float UpperSetYaw, int32_t UpperSetSpeed)
{
    PID_PositionCalc(&YawPID, yaw, UpperSetYaw);
    LeftForwardSpeed = (int32_t)(UpperSetSpeed - YawPID.U / 10  );
    RightForwardSpeed = (int32_t)(UpperSetSpeed + YawPID.U / 10 );

    PID_SpeedCalc(&LFMotorPID, MotorSpeed3, LeftForwardSpeed);
    LFtemp += (int32_t)LFMotorPID.U;
    MotorSpeedLF(LFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&RFMotorPID, -MotorSpeed1, RightForwardSpeed);
    RFtemp += (int32_t)RFMotorPID.U;
    MotorSpeedRF(RFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&LBMotorPID, -MotorSpeed2, LeftForwardSpeed);
    LBtemp += (int32_t)LBMotorPID.U;
    MotorSpeedLB(LBtemp);  //�����ת�ٿ���

    PID_SpeedCalc(&RBMotorPID, -MotorSpeed4, RightForwardSpeed);
    RBtemp += (int32_t)RBMotorPID.U;
    MotorSpeedRB(RBtemp);  //�Һ���ת�ٿ���


}

void GyrateRight(void)
{
    PID_SpeedCalc(&LFMotorPID, MotorSpeed3, 30);
    LFtemp += (int32_t)LFMotorPID.U;
    MotorSpeedLF(LFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&RFMotorPID, -MotorSpeed1, -30);
    RFtemp += (int32_t)RFMotorPID.U;
    MotorSpeedRF(RFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&LBMotorPID, -MotorSpeed2, 30);
    LBtemp += (int32_t)LBMotorPID.U;
    MotorSpeedLB(LBtemp);  //�����ת�ٿ���

    PID_SpeedCalc(&RBMotorPID, -MotorSpeed4, -30);
    RBtemp += (int32_t)RBMotorPID.U;
    MotorSpeedRB(RBtemp);  //�Һ���ת�ٿ���
}

void GyrateLeft(void)
{
    PID_SpeedCalc(&RFMotorPID, -MotorSpeed1, -30);
    RFtemp += (int32_t)RFMotorPID.U;
    MotorSpeedRF(RFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&LBMotorPID, -MotorSpeed2, 30);
    LBtemp += (int32_t)LBMotorPID.U;
    MotorSpeedLB(LBtemp);  //�����ת�ٿ���

    PID_SpeedCalc(&LFMotorPID, MotorSpeed3, -30);
    LFtemp += (int32_t)LFMotorPID.U;
    MotorSpeedLF(LFtemp);  //��ǰ���ת�ٿ���

    PID_SpeedCalc(&RBMotorPID, -MotorSpeed4, 30);
    RBtemp += (int32_t)RBMotorPID.U;
    MotorSpeedRB(RBtemp);  //�Һ���ת�ٿ���
}
