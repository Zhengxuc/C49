#ifndef QMI8658A_H
#define QMI8658A_H


#include <stdio.h>
#include <string.h>

enum Qmi8658AReg
{
	Register_WhoAmI = 0,
	Register_Revision,
	Register_Ctrl1,
	Register_Ctrl2,
	Register_Ctrl3,
	Register_Reserved,
	Register_Ctrl5,
	Register_Reserved1,
	Register_Ctrl7,
	Register_Ctrl8,
	Register_Ctrl9,
	Register_Cal1_L = 11,
	Register_Cal1_H,
	Register_Cal2_L,
	Register_Cal2_H,
	Register_Cal3_L,
	Register_Cal3_H,
	Register_Cal4_L,
	Register_Cal4_H,
	Register_FifoWmkTh = 19, 
	Register_FifoCtrl = 20,
	Register_FifoCount = 21,
	Register_FifoStatus = 22,
	Register_FifoData = 23,
	Register_StatusInt = 45,
	Register_Status0,
	Register_Status1,
	Register_Timestamp_L = 48,
	Register_Timestamp_M,
	Register_Timestamp_H,
	Register_Tempearture_L = 51,
	Register_Tempearture_H,
	Register_Ax_L = 53,
	Register_Ax_H,
	Register_Ay_L,
	Register_Ay_H,
	Register_Az_L,
	Register_Az_H,
	Register_Gx_L = 59,
	Register_Gx_H,
	Register_Gy_L,
	Register_Gy_H,
	Register_Gz_L,
	Register_Gz_H,
	Register_COD_Status = 70,
	Register_dQW_L = 73,
	Register_dQW_H,
	Register_dQX_L,
	Register_dQX_H,
	Register_dQY_L,
	Register_dQY_H,
	Register_dQZ_L,
    Register_dQZ_H,
    Register_dVX_L,
	Register_dVX_H,
	Register_dVY_L,
	Register_dVY_H,
	Register_dVZ_L,
    Register_dVZ_H,

	Register_TAP_Status = 89,
	Register_Step_Cnt_L = 90,
	Register_Step_Cnt_M = 91,
    Register_Step_Cnt_H = 92,

	Register_Reset = 96
};

//详细说明参照note.md Ctrl9详细命令说明
enum Ctrl9Command
{
	Ctrl9_Cmd_Ack					= 0X00,
	Ctrl9_Cmd_RstFifo				= 0X04,
	Ctrl9_Cmd_ReqFifo				= 0X05,// Get FIFO data from Device 
	Ctrl9_Cmd_WoM_Setting			= 0x08,// 设置并启用运动唤醒
	Ctrl9_Cmd_AccelHostDeltaOffset	= 0x09,// 更改加速度计偏移
	Ctrl9_Cmd_GyroHostDeltaOffset	= 0x0A,// 更改陀螺仪偏移
	Ctrl9_Cmd_CfgTap				= 0x0C,// 配置TAP检测
	Ctrl9_Cmd_CfgPedometer  		= 0x0D,// 配置计步器
	Ctrl9_Cmd_Motion				= 0x0E,// 配置任何运动/无运动/显着运动检测
    Ctrl9_Cmd_RstPedometer          = 0x0F,// 重置计步器计数（步数）
	Ctrl9_Cmd_CopyUsid				= 0x10,// 将 USID 和 FW 版本复制到 UI 寄存器
	Ctrl9_Cmd_SetRpu				= 0x11,// 配置 IO 上拉
    Ctrl9_Cmd_AHBClockGating        = 0x12,// 内部 AHB 时钟门控开关
	Ctrl9_Cmd_OnDemandCalivration	= 0xA2,// 陀螺仪按需校准
	Ctrl9_Cmd_ApplyGyroGains    	= 0xAA // 恢复保存的陀螺仪增益
};


#endif