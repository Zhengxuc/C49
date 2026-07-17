/*
 * @Author: XinXin
 * @Date: 2023-06-21 21:55:59
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-10 20:21:48
 * @FilePath: \source\language\language.h
 * @Description:
 */
#ifndef language_h__
#define language_h__

#include "common.h"

enum LANGUAGE
{
    LANGUAGE_CH = 0,
    LANGUAGE_EN,
    LANGUAGE_END,
};

struct languageData
{
    char const *index;
    char const *value;
};

struct language
{
    // lv_font_t const *font_lib;
    char const *name;
    char const *nation;
    struct languageData const *font;
    int len;
};

char const *tr(char const *str);
int get_language(void);
void set_language(enum LANGUAGE lu);

#endif
