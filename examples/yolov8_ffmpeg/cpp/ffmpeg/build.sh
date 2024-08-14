# Copyright (C) 2024 wwhai
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

#!/bin/bash

# 设置 FFmpeg 的路径
FFMPEG_DIR="/home/username/workspace/ffmpeg-6.1.1-full_build-shared"

# 设置包含路径和库路径
INCLUDE_PATH="$FFMPEG_DIR/include"
LIB_PATH="$FFMPEG_DIR/lib"

# 编译并链接
gcc -o rtsp_example rtsp_example.c \
-I"$INCLUDE_PATH" \
-L"$LIB_PATH" \
-lavformat -lavcodec -lavutil -lswscale -lm

# 检查编译是否成功
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
else
    echo "Compilation successful"
fi
