#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>

int main(int argc, char *argv[])
{
    const char *input_url = "rtsp://192.168.10.244:554/av0_0";
    const char *output_url = "rtmp://192.168.10.163:1935/live/test002";

    AVFormatContext *input_fmt_ctx = NULL, *output_fmt_ctx = NULL;
    AVCodecContext *decoder_ctx = NULL, *encoder_ctx = NULL;
    AVFilterGraph *filter_graph = NULL;
    AVFilterContext *buffersrc_ctx = NULL, *buffersink_ctx = NULL;
    AVFilterContext *drawbox_ctx = NULL;
    AVPacket packet;
    AVFrame *frame = NULL;
    AVFrame *filt_frame = NULL;
    int video_stream_index = -1;
    int ret;

    // Open input stream
    if ((ret = avformat_open_input(&input_fmt_ctx, input_url, NULL, NULL)) < 0)
    {
        fprintf(stderr, "Could not open input file.\n");
        return ret;
    }

    // Find stream info
    if ((ret = avformat_find_stream_info(input_fmt_ctx, NULL)) < 0)
    {
        fprintf(stderr, "Could not find stream information.\n");
        return ret;
    }

    // Find video stream
    for (int i = 0; i < input_fmt_ctx->nb_streams; i++)
    {
        if (input_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1)
    {
        fprintf(stderr, "Could not find a video stream in the input file.\n");
        return -1;
    }

    // Get decoder context
    const AVCodec *decoder = avcodec_find_decoder(input_fmt_ctx->streams[video_stream_index]->codecpar->codec_id);
    decoder_ctx = avcodec_alloc_context3(decoder);
    avcodec_parameters_to_context(decoder_ctx, input_fmt_ctx->streams[video_stream_index]->codecpar);
    if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0)
    {
        fprintf(stderr, "Could not open decoder.\n");
        return ret;
    }

    // Initialize filter graph
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
    {
        fprintf(stderr, "Unable to create filter graph.\n");
        return AVERROR(ENOMEM);
    }

    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt,
             decoder_ctx->time_base.num, decoder_ctx->time_base.den,
             decoder_ctx->sample_aspect_ratio.num, decoder_ctx->sample_aspect_ratio.den);

    // Create buffer source filter
    ret = avfilter_graph_create_filter(&buffersrc_ctx, avfilter_get_by_name("buffer"), "in",
                                       args, NULL, filter_graph);
    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Cannot create buffer source: %s\n", errbuf);
        return ret;
    }

    // Create drawbox filter
    const char *filter_descr = "drawbox=x=50:y=50:w=200:h=100:color=red@0.5:t=5";
    ret = avfilter_graph_create_filter(&drawbox_ctx, avfilter_get_by_name("drawbox"), "drawbox",
                                       filter_descr, NULL, filter_graph);
    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Cannot create drawbox filter: %s\n", errbuf);
        return ret;
    }

    // Create buffer sink filter
    ret = avfilter_graph_create_filter(&buffersink_ctx, avfilter_get_by_name("buffersink"), "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Cannot create buffer sink: %s\n", errbuf);
        return ret;
    }

    // Link filters
    ret = avfilter_link(buffersrc_ctx, 0, drawbox_ctx, 0);
    if (ret >= 0)
        ret = avfilter_link(drawbox_ctx, 0, buffersink_ctx, 0);
    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Error linking filters: %s\n", errbuf);
        return ret;
    }

    // Configure filter graph
    ret = avfilter_graph_config(filter_graph, NULL);
    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Error configuring the filter graph: %s\n", errbuf);
        return ret;
    }

    // Create output stream
    avformat_alloc_output_context2(&output_fmt_ctx, NULL, "flv", output_url);
    AVStream *out_stream = avformat_new_stream(output_fmt_ctx, NULL);
    if (!out_stream)
    {
        fprintf(stderr, "Failed allocating output stream.\n");
        return -1;
    }

    // Set encoder
    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    encoder_ctx = avcodec_alloc_context3(encoder);
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_ctx->time_base = (AVRational){1, 25};
    if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        encoder_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if ((ret = avcodec_open2(encoder_ctx, encoder, NULL)) < 0)
    {
        fprintf(stderr, "Cannot open encoder: %s\n", av_err2str(ret));
        return ret;
    }

    // Set output stream's encoder parameters
    if ((ret = avcodec_parameters_from_context(out_stream->codecpar, encoder_ctx)) < 0)
    {
        fprintf(stderr, "Failed to copy encoder parameters to output stream.\n");
        return ret;
    }

    // Open output URL
    if (!(output_fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        if ((ret = avio_open(&output_fmt_ctx->pb, output_url, AVIO_FLAG_WRITE)) < 0)
        {
            fprintf(stderr, "Cannot open output URL: %s\n", av_err2str(ret));
            return ret;
        }
    }

    // Write file header
    if ((ret = avformat_write_header(output_fmt_ctx, NULL)) < 0)
    {
        fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
        return ret;
    }

    // Allocate frames
    frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    if (!frame || !filt_frame)
    {
        fprintf(stderr, "Could not allocate frames.\n");
        return AVERROR(ENOMEM);
    }

    while (1)
    {
        if ((ret = av_read_frame(input_fmt_ctx, &packet)) < 0)
            break;
        if (packet.stream_index == video_stream_index)
        {
            avcodec_send_packet(decoder_ctx, &packet);
            while (avcodec_receive_frame(decoder_ctx, frame) == 0)
            {
                frame->pts = frame->best_effort_timestamp;

                // Add frame to filter graph
                if ((ret = av_buffersrc_add_frame(buffersrc_ctx, frame)) < 0)
                {
                    fprintf(stderr, "Error while feeding the filtergraph: %s\n", av_err2str(ret));
                    return ret;
                }

                // Get processed frame
                while ((ret = av_buffersink_get_frame(buffersink_ctx, filt_frame)) >= 0)
                {
                    // Encode processed frame
                    avcodec_send_frame(encoder_ctx, filt_frame);
                    AVPacket enc_pkt;
                    av_packet_make_refcounted(&enc_pkt);
                    while (avcodec_receive_packet(encoder_ctx, &enc_pkt) == 0)
                    {
                        // Write encoded packet to output stream
                        av_write_frame(output_fmt_ctx, &enc_pkt);
                        av_packet_unref(&enc_pkt);
                    }
                    av_frame_unref(filt_frame);
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(&packet);
    }

    // Write file trailer
    av_write_trailer(output_fmt_ctx);

    // Cleanup
    if (output_fmt_ctx && !(output_fmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_fmt_ctx->pb);
    avformat_free_context(output_fmt_ctx);
    avfilter_graph_free(&filter_graph);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    return 0;
}
