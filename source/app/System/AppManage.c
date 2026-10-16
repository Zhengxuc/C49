#include "AppManage.h"
#include "common.h"
#include <stdbool.h>

static int task = 0;
static bool GUI_Collision = true;

void appManage_only(void (*void_dat)(void)) //º¯ÊýÖ¸Õë
{
    if (GUI_Collision)
    {
        void_dat();
        GUI_Collision = false;
    }
}

void appManage_switchTask(enum AppTask p)
{
    GUI_Collision = true;
    task = p;
}

inline enum AppTask appManage_getTask(void)
{
    return task;
}
