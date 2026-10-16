/*
 * @Author: XinXin
 * @Date: 2024-03-28 20:58:57
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-08 15:09:26
 * @FilePath: \source\app\System\AppManage.h
 * @Description:
 */

#ifndef __APPMANAGE_H__
#define __APPMANAGE_H__

enum AppTask
{
    APP_TASK_NONE = 0,//没有任务
    APP_TASK_AUTO_ADJ,//自动整定
    APP_TASK_AUTO_CONTROL,//自动控制
    APP_TASK_PRESSURE,//压力？
};

void appManage_only(void (*void_dat)(void));
void appManage_switchTask(enum AppTask p);
inline enum AppTask appManage_getTask(void);

#endif
