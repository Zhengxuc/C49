
#include "buffer.h"
#include <string.h>
#include <stdio.h>

// 初始化缓冲区
void Buffer_Init(IntBuffer_t *buffer, int *bufferData, int bufferSize)
{
    buffer->size = bufferSize;
    buffer->buffer = bufferData;
    buffer->head = 0;
    buffer->tail = 0;
    memset(buffer->buffer, 0, buffer->size);
}

// 向缓冲区添加数据
void Buffer_Push(IntBuffer_t *buffer, int data)
{
    // 计算下一个插入位置
    int next = (buffer->tail + 1) % (buffer->size);

    // 如果缓冲区已满，覆盖最旧的数据
    if (next == buffer->head)
    {
        buffer->head = (buffer->head + 1) % (buffer->size);
    }

    // 插入新数据
    buffer->buffer[buffer->tail] = data;
    buffer->tail = next;
}

// 检查缓冲区是否为空
int Buffer_IsEmpty(IntBuffer_t *buffer)
{
    return buffer->head == buffer->tail;
}

// 清空缓冲区
void Buffer_Clear(IntBuffer_t *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
    memset(buffer->buffer, 0, buffer->size);
}

/*获取数据有多少*/
int Buffer_Length(IntBuffer_t *buffer)
{
    int length;

    if (buffer->head <= buffer->tail)
    {
        length = buffer->tail - buffer->head;
    }
    else
    {
        length = (buffer->size) - (buffer->head - buffer->tail);
    }

    return length;
}

// 从缓冲区获取最开始的数据,获取数据后会移动指针
int Buffer_Pop(IntBuffer_t *buffer)
{
    // 如果缓冲区为空，返回错误或特定值
    if (buffer->head == buffer->tail)
    {
        // 可以选择返回一个错误码或特定值
        return -1;
    }
    // 取出数据
    int data = buffer->buffer[buffer->head];
    // 移动头指针
    buffer->head = (buffer->head + 1) % (buffer->size);
    return data;
}

/*查看数据,从最后加入的数据往前计算position个*/
int Buffer_Peek_back(IntBuffer_t *buffer, int position)
{
    // 计算实际数组索引
    int index = ((buffer->tail - 1) - position);
    if (index < 0)
    {
        index = (buffer->size) + index;
    }

    // 返回数据
    return buffer->buffer[index];
}

// 打印缓冲区内容
void Buffer_Print(IntBuffer_t *buffer)
{
    if (Buffer_IsEmpty(buffer))
    {
        printf("Buffer is empty.\n");
        return;
    }
    printf("Buffer contents: ");
    for (int i = buffer->head; i != buffer->tail; i = (i + 1) % (buffer->size))
    {
        printf("%d ", buffer->buffer[i]);
    }
    printf("\n");
}