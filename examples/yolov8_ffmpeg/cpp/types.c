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
#ifndef TYPES
#define TYPES
#include <pthread.h>

#define TNew(_T_, _V_) _T_ *_V_ = (_T_ *)malloc(sizeof(_T_));

pthread_mutex_t thread_mutex;
pthread_cond_t queue_not_empty;

typedef unsigned char TBool;

enum
{
    TFalse = 0,
    TTrue = 1
};
#endif