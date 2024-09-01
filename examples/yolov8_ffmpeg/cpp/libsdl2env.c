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
#ifndef SDL2ENV
#define SDL2ENV
extern "C"
{
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libavformat/avformat.h>
}

#include "types.c"
#include "queue.c"
#include <pthread.h>

typedef struct TLibSDL2Env
{
    SDL_Window *mainWindow;
    SDL_Renderer *mainRenderer;
    SDL_Texture *mainTexture;
    TTF_Font *mainFont;

} TLibSDL2Env;
void DestroySDL2Env(TLibSDL2Env *Env);

TLibSDL2Env *NewLibSdl2Env()
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO |
                 SDL_INIT_TIMER | SDL_INIT_EVENTS))
    {
        printf("SDL_Init Error: %s\n", TTF_GetError());
        return NULL;
    }
    // if (TTF_Init() != 0)
    // {
    //     printf("TTF_Init Error: %s\n", TTF_GetError());
    //     SDL_Quit();
    //     return NULL;
    // }
    TNew(TLibSDL2Env, Sdl2Env);

    return Sdl2Env;
}

void TLibSDL2EnvDisplayFrame(TLibSDL2Env *Env, AVFrame *OneFrame)
{
    SDL_UpdateYUVTexture(Env->mainTexture, NULL,
                         OneFrame->data[0], OneFrame->linesize[0],
                         OneFrame->data[1], OneFrame->linesize[1],
                         OneFrame->data[2], OneFrame->linesize[2]);
    SDL_Rect srcRect = {0, 0, 1920, 1080};
    SDL_Rect distRect = {0, 0, 1920, 1080};
    SDL_RenderCopy(Env->mainRenderer, Env->mainTexture, &srcRect, &distRect);
}

int InitTLibSDL2Env(TLibSDL2Env *Env, int w, int h)
{
    Env->mainWindow = SDL_CreateWindow("FSY PLAYER",
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       w, h, SDL_WINDOW_SHOWN);
    if (!Env->mainWindow)
    {
        printf("SDL_CreateWindow Error: %s\n", TTF_GetError());
        return -1;
    }
    Env->mainRenderer = SDL_CreateRenderer(Env->mainWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!Env->mainRenderer)
    {
        printf("SDL_CreateWindow Error: %s\n", TTF_GetError());
        return -1;
    }
    Env->mainTexture = SDL_CreateTexture(Env->mainRenderer,
                                         SDL_PIXELFORMAT_IYUV,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         w, h);
    if (!Env->mainTexture)
    {
        printf("SDL_CreateTexture Error: %s\n", TTF_GetError());
        return -1;
    }
    // Env->mainFont = TTF_OpenFont("Duran-Medium.ttf", 24);
    // if (!Env->mainFont)
    // {
    //     printf("TTF_OpenFont Error: %s\n", TTF_GetError());
    //     return -1;
    // }
    return 0;
}
/// @brief 显示
/// @param Env
/// @param DisplayFrame
void TLibSDL2EnvEventLoop(TLibSDL2Env *Env, Queue *Queue)
{
    SDL_Event e;
    int running = 1;
    while (running)
    {

        //------------------------------------------------------------------------------------------
        SDL_RenderClear(Env->mainRenderer);
        SDL_SetRenderDrawColor(Env->mainRenderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
        while (!isQueueEmpty(Queue))
        {
            QueueData data = dequeue(Queue);
            printf("QueueData data.frame->pkt_size: %d\n", data.frame->width);
        }

        SDL_RenderPresent(Env->mainRenderer);
        //------------------------------------------------------------------------------------------

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = 0;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = 0;
                }
            }
            if ((SDL_GetTicks() - SDL_GetTicks()) < 10)
            {
                SDL_Delay(10);
            }
        }
    }
    DestroySDL2Env(Env);
}
void DestroySDL2Env(TLibSDL2Env *Env)
{
    SDL_DestroyTexture(Env->mainTexture);
    SDL_DestroyRenderer(Env->mainRenderer);
    SDL_DestroyWindow(Env->mainWindow);
    TTF_Quit();
    SDL_Quit();
}

#endif