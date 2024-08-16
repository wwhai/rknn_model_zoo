#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "yolov8.h"
#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"
#include "im2d.h"
void send_avframe_to_yolo8(int yolo8, AVFrame *frame)
{
    printf("send_avframe_to_yolo8\n");
}
void receive_result_from_yolo8(int yolo8, void *p)
{
    printf("receive_result_from_yolo8\n");
}
int main(int argc, char *argv[])
{
    int ret;
    const char *input_url = "rtsp://192.168.10.244:554/av0_0";
    const char *output_url = "rtmp://127.0.0.1:1935/live/test001";
    const char *model_path = "";

    rknn_app_context_t rknn_app_ctx;
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    init_post_process();

    ret = init_yolov8_model(model_path, &rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_yolov8_model fail! ret=%d model_path=%s\n", ret, model_path);
        deinit_post_process();
        release_yolov8_model(&rknn_app_ctx);
        return 0;
    }

    AVFormatContext *input_fmt_ctx = NULL, *output_fmt_ctx = NULL;
    AVCodecContext *input_codec_ctx = NULL, *output_codec_ctx = NULL;
    AVStream *video_stream = NULL;
    AVPacket packet;
    AVFrame *frame = NULL;
    AVFrame *rgb_frame = NULL;
    int video_stream_index = -1;
    int ret;

    // Open the input RTSP stream
    if (avformat_open_input(&input_fmt_ctx, input_url, NULL, NULL) < 0)
    {
        fprintf(stderr, "Could not open input stream\n");
        return 1;
    }

    if (avformat_find_stream_info(input_fmt_ctx, NULL) < 0)
    {
        fprintf(stderr, "Could not find stream info\n");
        return 1;
    }

    // Find the first video stream
    for (int i = 0; i < input_fmt_ctx->nb_streams; i++)
    {
        if (input_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index < 0)
    {
        fprintf(stderr, "No video stream found\n");
        return 1;
    }

    // Find the decoder for the video stream
    AVCodecParameters *codecpar = input_fmt_ctx->streams[video_stream_index]->codecpar;
    const AVCodec *input_codec = avcodec_find_decoder(codecpar->codec_id);
    if (!input_codec)
    {
        fprintf(stderr, "Codec not found\n");
        return 1;
    }

    // Allocate a codec context for the decoder
    input_codec_ctx = avcodec_alloc_context3(input_codec);
    if (!input_codec_ctx)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        return 1;
    }

    // Copy codec parameters from input stream to codec context
    if (avcodec_parameters_to_context(input_codec_ctx, codecpar) < 0)
    {
        fprintf(stderr, "Could not copy codec parameters to context\n");
        return 1;
    }

    // Open the codec
    if (avcodec_open2(input_codec_ctx, input_codec, NULL) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        return 1;
    }

    // Create the output format context for RTMP
    if (avformat_alloc_output_context2(&output_fmt_ctx, NULL, "flv", output_url) < 0)
    {
        fprintf(stderr, "Could not create output context\n");
        return 1;
    }

    // Create a new video stream in the output context
    video_stream = avformat_new_stream(output_fmt_ctx, NULL);
    if (!video_stream)
    {
        fprintf(stderr, "Could not create new stream\n");
        return 1;
    }

    // Find and open the encoder for the output stream
    const AVCodec *output_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!output_codec)
    {
        fprintf(stderr, "Encoder not found\n");
        return 1;
    }

    output_codec_ctx = avcodec_alloc_context3(output_codec);
    if (!output_codec_ctx)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        return 1;
    }

    output_codec_ctx->height = input_codec_ctx->height;
    output_codec_ctx->width = input_codec_ctx->width;
    output_codec_ctx->sample_aspect_ratio = input_codec_ctx->sample_aspect_ratio;
    output_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    output_codec_ctx->time_base = (AVRational){1, 25}; // Assume 25 fps for simplicity

    if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
        output_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(output_codec_ctx, output_codec, NULL) < 0)
    {
        fprintf(stderr, "Could not open encoder\n");
        return 1;
    }

    // Copy codec parameters from codec context to output stream
    if (avcodec_parameters_from_context(video_stream->codecpar, output_codec_ctx) < 0)
    {
        fprintf(stderr, "Could not copy codec parameters to output stream\n");
        return 1;
    }

    // Open the output file
    if (!(output_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&output_fmt_ctx->pb, output_url, AVIO_FLAG_WRITE) < 0)
        {
            fprintf(stderr, "Could not open output file\n");
            return 1;
        }
    }

    // Write the stream header
    if (avformat_write_header(output_fmt_ctx, NULL) < 0)
    {
        fprintf(stderr, "Error occurred while writing header\n");
        return 1;
    }

    frame = av_frame_alloc();
    rgb_frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate frame\n");
        return 1;
    }
    struct SwsContext *sws_ctx = sws_getContext(
        input_codec_ctx->width, input_codec_ctx->height, input_codec_ctx->pix_fmt,
        640, 640, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, NULL, NULL, NULL);

    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24,
                                             input_codec_ctx->width,
                                             input_codec_ctx->height, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(num_bytes);
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, buffer, AV_PIX_FMT_RGB24,
                         input_codec_ctx->width, input_codec_ctx->height, 1);

    while (av_read_frame(input_fmt_ctx, &packet) >= 0)
    {
        if (packet.stream_index == video_stream_index)
        {
            if (avcodec_send_packet(input_codec_ctx, &packet) < 0)
            {
                fprintf(stderr, "Error sending packet to decoder\n");
                av_packet_unref(&packet);
                continue;
            }
            while (avcodec_receive_frame(input_codec_ctx, frame) >= 0)
            {
                //-------------------------------------------------------------
                // 格式转换成RGB，640*640
                //-------------------------------------------------------------
                sws_scale(sws_ctx, (const uint8_t *const *)frame->data,
                          frame->linesize, 0, input_codec_ctx->height,
                          rgb_frame->data, rgb_frame->linesize);
                image_buffer_t src_image;
                memset(&src_image, 0, sizeof(image_buffer_t));
                src_image.width = 640;
                src_image.height = 640;
                src_image.width_stride = 0;
                src_image.height_stride = 0;
                src_image.size = sizeof(num_bytes);
                src_image.virt_addr = (unsigned char *)rgb_frame->data[0];
                src_image.format = IMAGE_FORMAT_RGB888;

                object_detect_result_list od_results;
                ret = inference_yolov8_model(&rknn_app_ctx, &src_image, &od_results);
                if (ret != 0)
                {
                    printf("init_yolov8_model fail! ret=%d\n", ret);
                    continue;
                }
                //-------------------------------------------------------------
                frame->pts = frame->best_effort_timestamp;
                if (avcodec_send_frame(output_codec_ctx, frame) < 0)
                {
                    fprintf(stderr, "Error sending frame to encoder\n");
                    continue;
                }
                send_avframe_to_yolo8(0, frame);
                receive_result_from_yolo8(0, NULL);
                //
                while (avcodec_receive_packet(output_codec_ctx, &packet) >= 0)
                {
                    AVStream *in_stream = input_fmt_ctx->streams[packet.stream_index];
                    AVStream *out_stream = output_fmt_ctx->streams[packet.stream_index];

                    packet.pts = av_rescale_q(packet.pts, in_stream->time_base, out_stream->time_base);
                    packet.dts = packet.pts;
                    packet.duration = packet.duration;
                    if (av_interleaved_write_frame(output_fmt_ctx, &packet) < 0)
                    {
                        fprintf(stderr, "Error writing packet to output file\n");
                    }

                    av_packet_unref(&packet);
                }
            }
        }
        av_packet_unref(&packet);
    }
    // FFMPEG
    av_write_trailer(output_fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    avcodec_free_context(&input_codec_ctx);
    avcodec_free_context(&output_codec_ctx);
    avformat_close_input(&input_fmt_ctx);
    avio_closep(&output_fmt_ctx->pb);
    avformat_free_context(output_fmt_ctx);
    // yolo8
    deinit_post_process();
    release_yolov8_model(&rknn_app_ctx);
    return 0;
}
