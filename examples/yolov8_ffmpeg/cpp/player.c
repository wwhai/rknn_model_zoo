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
#ifndef PLAYER
#define PLAYER

#include <pthread.h>
#include "libavenv.c"
#include "queue.c"
#include "types.c"
typedef struct
{
    TLibAVEnv *AvEnv;
    Queue *queue;
} TPlayer;

TPlayer *NewTPlayer()
{
    TNew(TPlayer, player);
    pthread_mutex_init(&thread_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    player->AvEnv = NewTLibAVEnv();
    player->queue = CreateQueue(100);
    return player;
}
/// @brief 初始化
/// @param player
/// @return
int TPlayerInit(TPlayer *player)
{
    int ret = -1;
    ret = TLibAVEnvInitInputCodec(player->AvEnv, "rtsp://192.168.10.244:554/av0_0");
    if (ret < 0)
    {
        return ret;
    }
    ret = TLibAVEnvInitOutputCodec(player->AvEnv, "rtmp://192.168.10.163:1935/live/test001");
    if (ret < 0)
    {
        return ret;
    }
    ret = TLibAVEnvInitAvFrame(player->AvEnv);
    if (ret < 0)
    {
        return ret;
    }
    ret = TLibAVEnvInitSWS(player->AvEnv);
    if (ret < 0)
    {
        return ret;
    }
    ret = TLibAVEnvInitModel(player->AvEnv);
    if (ret < 0)
    {
        return ret;
    }
    return 1;
}
/// @brief 线程函数
/// @param data
/// @return
void *LibSdlThreadCallback(void *data)
{
    TPlayer *player = (TPlayer *)data;
    char stringbuf[100];
    while (1)
    {
        pthread_mutex_lock(&thread_mutex); // 加锁
        if (!isEmpty(player->queue))
        {
            QueueData data = dequeue(player->queue);
            if (data.frame != NULL)
            {
                sprintf(stringbuf, "Frame: %p, X: %d, Y: %d, Width: %d, Height: %d, Label: %s:%2f",
                        data.frame, data.x, data.y, data.w, data.h, data.label, data.prop);
                printf("@@@@@@@@@@ LibSdlThreadCallback object detect result: %s\n", stringbuf);
            }
        }
        pthread_mutex_unlock(&thread_mutex); // 解锁
    }
    pthread_exit(NULL);
}
/// @brief 线程函数
/// @param data
/// @return
void *LibAvThreadCallback(void *data)
{
    TPlayer *player = (TPlayer *)data;
    TLibAVEnvReceiveDisplay(player->AvEnv, player->queue);
    pthread_exit(NULL);
}
/// @brief 启动线程
/// @param player
void StartTPlayer(TPlayer *player)
{
    void *ret_val;
    //
    pthread_t LibAvThread;
    pthread_t LibSdlThread;
    //
    pthread_create(&LibAvThread, NULL, &LibAvThreadCallback, (void *)player);
    pthread_create(&LibSdlThread, NULL, &LibSdlThreadCallback, (void *)player);
    // //
    pthread_detach(LibAvThread);
    pthread_detach(LibSdlThread);
}
/// @brief 停止
/// @param player
void StopTPlayer(TPlayer *player)
{
    if (player->AvEnv)
    {
        DestroyTLibAVEnv(player->AvEnv);
    }
    if (player->queue != NULL)
    {
        freeQueue(player->queue);
    }
    pthread_mutex_destroy(&thread_mutex);
    pthread_cond_destroy(&queue_not_empty);
}
#endif