/*
 * @Author: XinXin
 * @Date: 2023-09-04 20:55:38
 * @LastEditors: XinXin
 * @LastEditTime: 2023-09-04 21:01:09
 * @FilePath: \source\language\language_en.c
 * @Description:
 */
#include "language.h"

static struct languageData const language_data[] = {
    {"", ""}, /*如果索引采用的英文，就不用翻译*/

    /*前面是索引，后面为翻译语言*/
};

struct language const language_en = {
    "sub",
    "English",
    language_data,
};
