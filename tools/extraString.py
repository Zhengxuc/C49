'''
Author: XinXin
Date: 2024-01-15 08:58:04
LastEditors: XinXin
LastEditTime: 2024-01-15 18:30:50
FilePath: \tools\extraString.py
Description: 提取文件中的中文字符串，匹配对象是双引号，双引号内的中文都会提取出来并过滤重复文字
'''

import sys
import os
import re
from collections import OrderedDict

 
#递归查找文件
def find_files(directory, target):
    result = []
    for file in os.listdir(directory):
        file_path = os.path.join(directory, file)
        if os.path.isfile(file_path):
            if target.lower() in file.lower():
                result.append(file_path)
        elif os.path.isdir(file_path):
            result.extend(find_files(file_path, target))
    return result

def is_chinese(s):
    """判断字符串是否为中文"""
    for c in s:
        if '\u4e00' <= c <= '\u9fff':
            return True
    return False

#main
args = sys.argv
if(len(args)<2):
    print("缺少文件名,文件名不区分大小写")
    exit()

# 查找文件
filename = find_files("../", args[1])
if(len(filename)==0):
    print("未找到这个文件")
    exit()
print(filename[0])

# 用utf-8编码打开文件
with open(filename[0], 'r', encoding='utf-8') as file:
    content = file.read()

# 使用正则表达式匹配双引号内的内容
matches = re.findall(r'"(.*?)"', content)

# 只保留中文
allChinese = ""
for match in matches:
    if(is_chinese(match)):
        allChinese += match

# 过滤重复文字
unique_chars = OrderedDict()
for char in allChinese:
    unique_chars[char] = None
result = ''.join(unique_chars.keys())
print(result)

