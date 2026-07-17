/*
 * @Author: XinXin
 * @Date: 2023-11-06 17:13:23
 * @LastEditors: XinXin
 * @LastEditTime: 2024-03-23 18:39:51
 * @FilePath: \source\common\DataPort.c
 * @Description:
 */

#include "DataPort.h"
#include <stdbool.h>

static const char *TAG = "DataPort";

struct DataPort
{
    bool flag;
    int32_t len;
    union
    {
        float32_t fdata;
        int32_t idata;
        void *data;
        const char *sdata;
    };
} uDataPort[DATA_MAX];

void port_set_error(void)
{
    DEBUG_LOGE(TAG, "The index length exceeds the limit");
}

void Port_DataInit(void)
{
    for (int i = 0; i < DATA_MAX; i++)
    {
        uDataPort[i].flag = false;
        uDataPort[i].len = 0;
        uDataPort[i].data = NULL;
    }
}

bool Port_Set(enum DataListIndex index, void *dat)
{
    if (index >= DATA_MAX)
        return false;
    uDataPort[index].flag = true;
    uDataPort[index].data = dat;
    return true;
}

bool Port_Get(void **data, enum DataListIndex index)
{
    bool state = uDataPort[index].flag;
    if (data != NULL)
        *data = uDataPort[index].data;
    uDataPort[index].flag = false;
    return state;
}

void Port_SetFloat(enum DataListIndex index, float32_t dat)
{
    if (!Port_Set(index, NULL))
    {
        port_set_error();
    }
    uDataPort[index].fdata = dat;
}

float32_t Port_GetFloat(enum DataListIndex index)
{
    Port_Get(NULL, index);
    return uDataPort[index].fdata;
}

void Port_SetInt(enum DataListIndex index, int32_t dat)
{
    if (!Port_Set(index, NULL))
    {
        port_set_error();
    }
    uDataPort[index].idata = dat;
}

int32_t Port_GetInt(enum DataListIndex index)
{
    Port_Get(NULL, index);
    return uDataPort[index].idata;
}

void Port_SetStr(enum DataListIndex index, const char *dat)
{
    if (!Port_Set(index, NULL))
    {
        port_set_error();
    }
    uDataPort[index].sdata = dat;
}

const char *Port_GetStr(enum DataListIndex index)
{
    if (Port_Get(NULL, index))
    {
        return uDataPort[index].sdata;
    }
    return NULL;
}
