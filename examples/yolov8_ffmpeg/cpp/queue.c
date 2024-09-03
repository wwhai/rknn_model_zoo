/**
 * Copyright (C) 2024 wwhai
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef QUEUE_H
#define QUEUE_H

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    AVFrame *frame;
    int x, y, w, h;
    float prop;
    char label[20];
} QueueData;

typedef struct Queue
{
    QueueData *array;
    int capacity;
    int front;
    int rear;
    int size;
} Queue;

Queue *CreateQueue(int capacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;
    queue->array = (QueueData *)malloc(queue->capacity * sizeof(QueueData));
    return queue;
}

int isFull(Queue *queue)
{
    return (queue->size == queue->capacity);
}

int isEmpty(Queue *queue)
{
    return (queue->size == 0);
}

void enqueue(Queue *queue, QueueData item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
}

QueueData dequeue(Queue *queue)
{
    if (isEmpty(queue))
        return (QueueData){NULL, 0, 0, 0, 0, 0.0, ""};
    QueueData item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

void freeQueue(Queue *queue)
{
    if (queue)
    {
        free(queue->array);
        free(queue);
    }
}

#endif
// 示例用法
// int main() {
//     Queue *queue = CreateQueue(10);

//     // 创建一个AVFrame（通常是从解码器获得的）
//     AVFrame *frame = av_frame_alloc();
//     // ... 设置frame的属性 ...

//     // 创建一个QueueData节点并入队
//     QueueData data = {frame, 10, 20, 100, 100, "example_label"};
//     enqueue(queue, data);

//     // 出队
//     QueueData dequeuedData = dequeue(queue);
//     // 使用dequeuedData中的数据
//     // ...

//     // 释放队列和AVFrame
//     freeQueue(queue);
//     av_frame_free(&frame);

//     return 0;
// }