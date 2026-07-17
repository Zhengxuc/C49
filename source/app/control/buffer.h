
#ifndef __BUFFER_H__
#define __BUFFER_H__

// 缓冲区结构体
typedef struct
{
    int size;    // 缓冲区大小
    int *buffer; // 存储数据的数组
    int head;    // 缓冲区头指针
    int tail;    // 缓冲区尾指针
} IntBuffer_t;

#include "buffer.h"

void Buffer_Init(IntBuffer_t *buffer, int *bufferData, int bufferSize);
void Buffer_Push(IntBuffer_t *buffer, int data);
int Buffer_IsEmpty(IntBuffer_t *buffer);
void Buffer_Clear(IntBuffer_t *buffer);
int Buffer_Length(IntBuffer_t *buffer);
int Buffer_Pop(IntBuffer_t *buffer);
int Buffer_Peek_back(IntBuffer_t *buffer, int position);
void Buffer_Print(IntBuffer_t *buffer);

#endif
