// Copyright (C) 2024 wwhai
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
extern "C"
{
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <libavformat/avformat.h>
}
#include "libsdl2env.c"
void DrawBox(SDL_Renderer *renderer, TTF_Font *font, const char *text,
             int x, int y, int w, int h, int thickness)
{
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
    SDL_Rect top = {x, y, w, thickness};
    SDL_RenderFillRect(renderer, &top);
    SDL_Rect bottom = {x, y + h - thickness, w, thickness};
    SDL_RenderFillRect(renderer, &bottom);
    SDL_Rect left = {x, y, thickness, h};
    SDL_RenderFillRect(renderer, &left);
    SDL_Rect right = {x + w - thickness, y, thickness, h};
    SDL_RenderFillRect(renderer, &right);
    if (text != NULL)
    {
        SDL_Color textColor = {255, 0, 0, 0};
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect;
        textRect.x = x;
        textRect.y = y - textSurface->h;
        textRect.w = textSurface->w;
        textRect.h = textSurface->h;
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }
}

int SDLDrawText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y)
{

    SDL_Color color = {255, 0, 0, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, color);
    if (textSurface == NULL)
    {
        SDL_Log("Unable to create text surface: %s", TTF_GetError());
        return -1;
    }
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == NULL)
    {
        SDL_Log("Unable to create texture from text surface: %s", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return -1;
    }
    SDL_Rect textRect = {x, y - 20, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    return 0;
}
