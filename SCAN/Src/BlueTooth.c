/**
  ******************************************************************************
  * @file    Blue_Tooth.h
  * @author  WangRong
  * @version V1.0
  * @date    2016.10.31
  * @note    ��������
  ******************************************************************************
  */
#include "BlueTooth.h"
#include "key.h"
#include <stdio.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
/**
  * @brief  ��uint8_t�������ݾ�����λ��
  * @param  NONE
  * @retval NONE
  * @attention
        ͨѶЭ��
    * @arg  ��һλ  0x00  ����
        * @arg  �ڶ�λ  key1~8
        * @arg  ����λ  key9~16
        * @arg  ����λ  key17~18
        * @arg  ����λ  0x0d  ����
        * @arg  ����λ  0x0a  ����
  */
void SendData(void)
{
    uint8_t Scan1, Scan2, Scan3;
    uint8_t (*Scan)[3];

    /*��������ָ�������ָ��*/
    Scan = KEY_Scan();
    //��ֵ
    Scan1 = (*Scan)[0];
    Scan2 = (*Scan)[1];
    Scan3 = (*Scan)[2];
    /*�ͷſռ�*/
    free(Scan);
    /*���ڷ���*/
    uint8_t SendData[6] = {0x00, Scan1, Scan2, Scan3, 0x0d, 0x0a};
    HAL_UART_Transmit(&huart1 , SendData, 6, 0xff);
}

