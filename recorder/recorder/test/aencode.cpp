#include "aencode.h"

AEncode::AEncode()
{
    memset(path,0,sizeof(path));
    pFormatCtx  = NULL;
    pVCodecCtx  = NULL;
    pVCodec     = NULL;
    fmt         = NULL;
    pVStream    = NULL;
    pVFrame     = av_frame_alloc();
    pVPacket    = av_packet_alloc();
    pSwsCtx     = NULL;
    out_buffer  = NULL;
    width       = 1920;
    height      = 1080;
    fps         = 25;
    videoindex  = 0;
    VPixFormat  = AV_PIX_FMT_YUV420P;
}

AEncode::~AEncode()
{
    free();
}

void AEncode::setPath(char *p)
{
    memset(path,0,sizeof(path));
    sprintf(path,"%s",p);
}
/*********************************
 * һ��Ҫ2�ı���
 ********************************/
void AEncode::setSize(int w, int h)
{
    width = w;
    height = h;
}

bool AEncode::initOutFile()
{
    //�ļ�·����û����
    if(!strlen(path)) {
        printf("file path is null.\n");
        return false;
    }
    //������ý��������
    if(avformat_alloc_output_context2(&pFormatCtx,NULL,NULL,path) < 0) {
        printf("alloc output context2 error.\n");
        return false;
    }

    fmt = pFormatCtx->oformat;

    //��ӡ��Ϣ
    //av_dump_format(pFormatCtx, 0, path, 1);
    //���ļ�
    if(!(fmt->flags & AVFMT_NOFILE)) {
        if(avio_open(&pFormatCtx->pb, path, AVIO_FLAG_WRITE) < 0) {
            printf("Could not open output file '%s'\n",path);
            return false;
        }
    }

    return true;
}

bool AEncode::initVideo()
{
    //���ұ�����
    pVCodec = avcodec_find_encoder(fmt->video_codec);
    if(!pVCodec) {
        printf("Could not find encoder.\n");
        return false;
    }
    //����������������
    pVCodecCtx = avcodec_alloc_context3(pVCodec);
    if(!pVCodecCtx) {
        printf("Could not alloc context.\n");
        return false;
    }
    //���ñ����������ĵĲ���
    pVCodecCtx->width = width;
    pVCodecCtx->height = height;
    pVCodecCtx->sample_aspect_ratio = AVRational{ width , height };  //�ֱ���

#if 0
    //������֧�־�ʹ�ñ������� ����Ĭ���� AV_PIX_FMT_YUV420P
    if(pVCodec->pix_fmts) pVCodecCtx->pix_fmt = pVCodec->pix_fmts[0];
    else pVCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
#else
    pVCodecCtx->pix_fmt = VPixFormat;
#endif

    pVCodecCtx->time_base = AVRational{ 1 , fps };                   //ʱ��
    pVCodecCtx->framerate = av_inv_q(pVCodecCtx->time_base);
    pVCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;                        //��Ƶ
//    pVCodecCtx->gop_size = 12;
//    pVCodecCtx->max_b_frames = 2;
    if(fmt->flags & AVFMT_GLOBALHEADER)
        pVCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *param = 0;
    av_dict_set(&param, "tune", "zerolatency", 0);      //ʵʱ���� ��������������д�����ļ�������
    //av_dict_set(&param, "preset", "slow", 0);
    //�򿪱�����
    if(avcodec_open2(pVCodecCtx,pVCodec,&param) < 0) {
        printf("Could not open encodec.\n");
        return false;
    }

    //������Ƶ��
    pVStream = avformat_new_stream(pFormatCtx,NULL);
    if(!pVStream) {
        printf("Cannot open video encoder for stream.\n");
        return false;
    }
    //����һЩ����
    if(avcodec_parameters_from_context(pVStream->codecpar,pVCodecCtx) < 0) {
        printf("Failed to copy encoder parameters to output stream.\n");
        return false;
    }
    return true;
}

bool AEncode::startEncodec()
{
    //д�ļ�ͷ
    if(avformat_write_header(pFormatCtx,NULL) < 0) {
        printf("Could not write header.\n");
        return false;
    }

    return true;
}
/**************
 * �������frame һ��Ҫ���ø߶� һ��Ҫ���� pts(����ʱ���)
 * pts �� 0 1 2 3 ... �ͺ���
 *************/
bool AEncode::encodecOneFrame(AVFrame *frame, int index)
{
    //ת��ͼ��
    sws_scale(pSwsCtx,(const unsigned char* const*)frame->data,frame->linesize,0,
              frame->height,pVFrame->data,pVFrame->linesize);
    pVFrame->format = VPixFormat;
    pVFrame->width = width;
    pVFrame->height = height;
    pVFrame->pts = frame->pts;

    int got_picture = 0;
    //����
    if(avcodec_encode_video2(pVCodecCtx,pVPacket,pVFrame,&got_picture) < 0) {
        printf("encode error.\n");
        av_packet_unref(pVPacket);
        return false;
    }
    //����������
    pVPacket->stream_index = videoindex;

    //���¼���pts
    av_packet_rescale_ts(pVPacket,pVCodecCtx->time_base,pVStream->time_base);

    //д��һ֡
    int ret = av_interleaved_write_frame(pFormatCtx,pVPacket);
    if(ret < 0) {
        printf("write frame ret %d.\n",ret);
        fflush(stdout);
        return false;
    }
    //�ͷſռ�
    av_packet_unref(pVPacket);
    return true;
}

bool AEncode::endEncodec()
{
    int ret = av_write_trailer(pFormatCtx);
    if (pFormatCtx && !(fmt->flags & AVFMT_NOFILE))
    {
        ret = avio_closep(&pFormatCtx->pb);
    }
    if(ret == 0)
        return true;
    printf("wirte trailer error %d.\n",ret);
    return false;
}

void AEncode::free()
{
    if(out_buffer)
        av_free(out_buffer);
    if(pSwsCtx)
        sws_freeContext(pSwsCtx);
    if(pVPacket)
        av_packet_free(&pVPacket);
    if(pVFrame)
        av_frame_free(&pVFrame);

//    if(pVStream)
//        av_free(pVStream);
    if(pVCodecCtx)
        avcodec_close(pVCodecCtx);

    if(pFormatCtx)
        avformat_free_context(pFormatCtx);
    pFormatCtx = NULL;
    pVCodecCtx = NULL;
    pVCodec = NULL;
    fmt = NULL;
    pVStream = NULL;
    pVFrame = NULL;
    pVPacket = NULL;
    pSwsCtx = NULL;
    out_buffer = NULL;
}

void AEncode::setSwsCtx(AVPixelFormat src, int src_w, int src_h)
{
    if(out_buffer)
        av_free(out_buffer);
    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(VPixFormat,width,height,1));
    av_image_fill_arrays(pVFrame->data,pVFrame->linesize,out_buffer,
                         VPixFormat,width,height,1);

    if(pSwsCtx)
        sws_freeContext(pSwsCtx);
    pSwsCtx = sws_getContext(src_w, src_h, src, width, height, VPixFormat,
                                 SWS_BICUBIC, NULL, NULL, NULL);

    //av_packet_free_side_data(pVPacket);
    av_new_packet(pVPacket,avpicture_get_size(VPixFormat ,width, height));
}
