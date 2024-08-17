gcc -o ffmpeg_rtsp_example.exe ffmpeg.c ^
    -IC:/Users/wangwenhai/workspace/ffmpeg-6.1.1-full_build-shared/include ^
    -LC:/Users/wangwenhai/workspace/ffmpeg-6.1.1-full_build-shared/bin ^
    -lavformat-60 -lavcodec-60 -lavutil-58 -lswscale-7 -lavfilter-9 -lm

gcc -o avfilter-demo.exe avfilter-demo.c ^
    -IC:/Users/wangwenhai/workspace/ffmpeg-6.1.1-full_build-shared/include ^
    -LC:/Users/wangwenhai/workspace/ffmpeg-6.1.1-full_build-shared/bin ^
    -lavformat-60 -lavcodec-60 -lavutil-58 -lswscale-7 -lavfilter-9 -lm
