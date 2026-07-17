/*
 * @Author: XinXin
 * @Date: 2023-03-18 09:24:05
 * @LastEditors: XinXin
 * @LastEditTime: 2023-04-09 21:47:23
 * @FilePath: \source\common\src\error.c
 * @Description:
 */

#include "error.h"
#include "debugLOG.h"

/**
 * @brief  抛出异常信息
 * @param  error 异常代码
 * @retval None
 */
void RaisingExceptions(ErrorCode error)
{
    static int i = -1;
    if (i != error)
    {
        i = error;
        if (error != NONE_ERROR)
        {
            DEBUG_LOGE("ERROR", "Error Code:%d", error);
        }
    }
}
