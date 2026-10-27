/*
 * @Author: XinXin
 * @Date: 2023-06-21 21:55:59
 * @LastEditors: XinXin
 * @LastEditTime: 2024-05-19 08:40:53
 * @FilePath: \source\page\language\language_zh.c
 * @Description:
 */
#include "language.h"

static struct languageData const language_data[] = {
    /*前面是索引，后面为翻译语言*/
    {"Language", "语言设置"},
    {"1.Auto", "1.自动参数"},
    {"2.Input", "2.输入参数"},
    {"3.Mechanical State", "3.机械状态参数"},
    {"4.Travel", "4.行程参数"},
    {"5.Control parameter", "5.控制参数"},
    {"6.PST", "6.定时任务"},
    {"7.Feedback", "7.阀门反馈参数"},
    {"8.System", "8.系统参数"},
    {"9.Auto ADJ", "9.自动整定"},

    {"In Value", "控制指令"},
    {"SetValue", "阀位位置"},
    {"Execution timeout", "行程时间极限"},
    {"Working mode", "安全位置选择"},
    {"Current calibration", "输入电流标定"},
    {"Language", "语言"},
    {"Display mirror", "显示反转"},
    {"Screen brightness", "屏幕亮度"},
    {"Screen Contrast ratio", "屏幕对比度"},
    {"Restore", "恢复出厂设置"},
    {"Auto", "自动"},
    {"Normal", "正常"},
    {"Reversal", "旋转"},

    {"Adjust current to", "调整电流为"},
    {"input value", "输入值"},
    {"End of calibration", "校准结束"},
    {"1.MinRange", "1.最小量程"},
    {"2.MaxRange", "2.最大量程"},

    {"Actuator type", "执行器类型"},
    {"Balance", "性能平衡"},
    {"Redundant error", "冗余误差"},
    {"frictionmeter", "阀门摩擦系数"},
    {"Line Mode", "直行程LT"},
    {"Angle Mode", "角行程ROTS"},
    {"MAX Speed", "最快速度"},
    {"Priority Speed", "速度优先"},
    {"Default", "适中"},
    {"Priority Stable", "稳定优先"},
    {"MAX Stable", "最佳稳定"},
    {"Low", "低摩擦"},
    {"High", "高摩擦"},
    {"Month", "月"},
    {"Hour", "小时"},
    {"Minute", "分"},
    {"Fail-safe", "故障安全"},
    {"Fail-lock", "故障闭锁"},
    {"Feedback direction", "阀位反馈方向"},
    {"Kp up", "上行Kp"},
    {"Ki up", "上行Ki"},
    {"Kd up", "上行Kd"},
    {"Kp down", "下行Kp"},
    {"Ki down", "下行Ki"},
    {"Kd down", "下行Kd"},

    {"Whether to save?", "是否保存？"},
    {"Yes", "是"},
    {"No", "否"},

    {"Input circuit MIN", "输入电流最小值"},
    {"Input circuit MAX", "输入电流最大值"},
    {"Control curve", "输入控制曲线"},
    {"Control Mode Select", "控制模式选择"},
    {"Closing speed", "调节速度"},
    {"Uplink time", "上行时间"},
    {"Down time", "下行时间"},
    {"User curve setting", "曲线设定"},
    {"Line", "线性"},
    {"User curve", "自定义"},
    {"Button Move", "按键手动"},
    {"Button Ratio", "按键比列"},
    {"Current Ratio", "电流比列"},
    {"RS485", "RS485"},
    {"HART", "HART"},

    {"Mechanical state MIN", "机械行程最小值"},
    {"Mechanical state MAX", "机械行程最大值"},
    {"Mechanical direction", "机械行程方向"},
    {"Positive going", "正向"},
    {"Reversed", "反向"},

    {"Control travel MIN", "控制行程最小值"},
    {"Control travel MAX", "控制行程最大值"},
    {"Travel direction", "位置显示反向"},
    {"Open", "打开"},
    {"Off", "关闭"},

    {"Release the button go menu", "松开按键进入菜单界面"},

    {"IP Parameter", "IP参数"},
    {"Travel Parameter", "行程参数"},
    {"Average inflation velocity", "充气速度平均值"},
    {"Average exhaust velocity", "排气速度平均值"},
    {"Inflation control column", "充气控制比列"},
    {"Exhaust control column", "排气控制比列"},
    {"Inflation overshoot distance", "充气过冲距离"},
    {"Exhaust overshoot distance", "排气过冲距离"},
    {"Inflation dead band", "充气死区"},
    {"Exhaust dead band", "排气死区"},

    {"Inflation minimum", "充气最小值"},
    {"Exhaust minimum", "排气最小值"},
    {"Inflation maximum", "充气最大值"},
    {"Exhaust maximum", "排气最大值"},
    {"Stable value", "稳定值"},
    {"Start of Travel ", "行程起点值"},
    {"End of Travel", "行程终点值"},
};

struct language const language_zh = {
    "思源黑体",
    "中国",
    language_data,
    sizeof(language_data),
};
