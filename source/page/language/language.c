/*
 * @Author: XinXin
 * @Date: 2023-06-21 21:55:59
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-10 20:22:04
 * @FilePath: \source\language\language.c
 * @Description:
 */

#include "language.h"
#include <stdio.h>
#include <string.h>

static int now_language = LANGUAGE_CH; // 默认语言 LANGUAGE_CH LANGUAGE_EN
static int translate_len = 0;          // 记录好翻译库词汇量

extern struct language const language_zh;
extern struct language const language_en;

struct language const *language_list[5] = {
    &language_zh,
    &language_en,
};

/*设置语言*/
void set_language(enum LANGUAGE lu)
{
    if (lu >= LANGUAGE_END)
        now_language = LANGUAGE_CH;
    else
        now_language = lu;
    translate_len = (language_list[now_language]->len) / sizeof(language_list[now_language]->font) / 2;
}

/*获取语言*/
int get_language(void)
{
    return now_language;
}

char const *tr(char const *str)
{
    for (int i = 0; i < translate_len; i++)
    {
        /*根据索引查找翻译语言*/
        if (strcmp(str, language_list[now_language]->font[i].index) == 0)
        {
            return language_list[now_language]->font[i].value;
        }
    }

    return str;
}
