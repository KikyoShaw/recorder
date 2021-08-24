#include "RecorderManager.h"
#include <QDebug>

RecorderManager::RecorderManager(QObject *parent /* = Q_NULLPTR */)
	:QObject(parent), m_isStop(false)
{

}

RecorderManager::~RecorderManager()
{
}

void RecorderManager::changeRecorderState(bool state)
{
	m_isStop = state;
}

void RecorderManager::startRecorderAudio()
{
	//保存路径
	QString path = "test.aac";

	FILE *outfile = fopen(path.toLatin1().data(), "wb+");

	//初始化paket
	int count = 0;
	AVPacket packet;
	av_init_packet(&packet);

	//重采样缓冲区
	uint8_t **src_data = NULL;
	int src_linesize = 0;
	uint8_t **dst_data = NULL;
	int dst_linesize = 0;

	//打开设备
	AVFormatContext* pFormatCtx = openDev();
	if (NULL == pFormatCtx)
		return;
	//打开编码器上下文
	AVCodecContext* pCodecCtx = openEncoder();
	if (NULL == pCodecCtx)
		return;
	//音频输入数据
	//创建编码器缓冲区
	AVFrame *pFrame = creatFrame();
	if (NULL == pFrame)
		return;
	//分配编码后的数据空间
	AVPacket *pNewpkt = av_packet_alloc();
	if (NULL == pNewpkt)
		return;
	//创建重采样上下文
	SwrContext* pSwrCtx = creatSwrContext();
	if (NULL == pNewpkt)
		return;
	//需要临时缓冲区采集一定量的数据之后再给编码器进行编码，因为单通道采样大小至少为32，pktsize为64 64/2(2通道)/2(16bit)<32
	int swr_num = 0;
	uint8_t *bufferData = new uint8_t[2048];
	alloc_data_resample(&src_data, &src_linesize, &dst_data, &dst_linesize);//分配重采样缓冲区

	auto ret = av_read_frame(pFormatCtx, &packet);
	while ((ret == 0) && m_isStop == false)// 从设备读取数据
	{
		av_log(NULL, AV_LOG_INFO, "Packet size: %d(%p), count = %d\n",
			packet.size, packet.data, count++);

		if (swr_num < 1984){
			for (int i = 0; i < packet.size; ++i){
				bufferData[i + swr_num] = packet.data[i];
			}
			swr_num += packet.size;
		}
		else{
			for (int i = 0; i < packet.size; ++i)
			{
				bufferData[i + swr_num] = packet.data[i];
			}
			swr_num = 0;
			qInfo() << QStringLiteral("重采样输入缓冲区大小 =") << src_linesize;
			memcpy((void*)src_data[0], (void *)bufferData, 2048);//读取设备音频数据缓冲区拷贝入重采样输入缓冲区

			swr_convert(pSwrCtx,
				dst_data,
				512,
				(const uint8_t **)src_data,
				512);
			qInfo() << QStringLiteral("重采样输出缓冲区大小 =") << dst_linesize;
			memcpy((void *)pFrame->data[0], (void *)dst_data[0], dst_linesize);//重采样输出缓冲区拷贝入编码器输入缓冲区

			encode(pCodecCtx, pFrame, pNewpkt, outfile);
		}
		// 释放packet空间
		av_packet_unref(&packet);
	}
	encode(pCodecCtx, NULL, pNewpkt, outfile);
	// 释放AVFrame 和 newpkt
	if (pFrame) {
		av_frame_free(&pFrame);
	}
	if (pNewpkt) {
		av_packet_free(&pNewpkt);
	}
	//释放输入输出缓冲区资源
	if (src_data) {
		av_freep(&src_data[0]);
	}
	av_freep(&src_data);

	if (dst_data) {
		av_freep(&dst_data[0]);
	}
	av_freep(&dst_data);
	//释放重采样上下文
	if (pSwrCtx) {
		swr_free(&pSwrCtx);
	}
	//释放编码器上下文
	if (pCodecCtx) {
		av_free(pCodecCtx);
	}
	//释放设备
	if (pFormatCtx) {
		avformat_close_input(&pFormatCtx);//releas ctx
	}
	if (outfile) {
		fclose(outfile);
	}
	av_log(NULL, AV_LOG_DEBUG, "finish!\n");
}

AVFormatContext * RecorderManager::openDev()
{
	//创建文件描述符
	AVFormatContext *pFormatCtx = NULL/*avformat_alloc_context()*/;
	AVInputFormat *pFormatInput = av_find_input_format("alsa");
	char *deviceName = "hw:0";

	//AVDictionary* opts = NULL;
	////设置tcp or udp，默认一般优先tcp再尝试udp
	//av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	////设置超时3秒
	//av_dict_set(&opts, "stimeout", "60000000", 0);

	//输入音频
	auto ret = avformat_open_input(&pFormatCtx, deviceName, pFormatInput, NULL/*&opts*/);
	if (ret < 0){
		//打印错误
		char errors[1024] = { 0 };
		av_strerror(ret, errors, 1024);
		qInfo() << "AVFormatContext init fail: " << ret << errors;
		return NULL;
	}

	qInfo() << "AVFormatContext init success!";
	return pFormatCtx;
}

AVCodecContext * RecorderManager::openEncoder()
{
	//按照编码器名称创建编码器
	AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
	//创建codec上下文
	if (codec) {
		//申请AVCodecContext空间
		AVCodecContext *pCodecCtx = avcodec_alloc_context3(codec);
		// 输入音频的采样大小。fdk_aac需要16位的音频输入数据	
		pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
		// 输入音频的CHANNEL LAYOUT
		pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		// 输入音频的声道数
		pCodecCtx->channels = 2;
		// 输入音频的采样率
		pCodecCtx->sample_rate = 44100;
		// AAC : 128K   AAV_HE: 64K  AAC_HE_V2: 32K. bit_rate为0时才会查找profile属性值
		pCodecCtx->bit_rate = 0;
		//profile属性值
		pCodecCtx->profile = FF_PROFILE_AAC_HE;
		// 打开编码器
		auto ret = avcodec_open2(pCodecCtx, codec, NULL);
		if (ret < 0) {
			qInfo() << "avcodec_open2 fail: " << ret;
			return NULL;
		}
		return pCodecCtx;
	}
	qInfo() << "avcodec_find_encoder_by_name fail: codec is NULL";
	return NULL;
}

void RecorderManager::encode(AVCodecContext * pCtx, AVFrame * pFrame, AVPacket * pkt, FILE * pOutput)
{
	//发送数据到编码器
	auto ret = avcodec_send_frame(pCtx, pFrame);
	if (ret < 0) {
		qInfo() << "avcodec_send_frame fail: " << ret;
		return;
	}
	//数据遍历写入文件
	while (ret >= 0){
		// 获取编码后的音频数据
		ret = avcodec_receive_packet(pCtx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
			return;
		}
		else if (ret < 0){
			qInfo() << "avcodec_receive_packet fail";
			exit(-1);
		}
		// 写入文件
		fwrite(pkt->data, 1, pkt->size, pOutput);
		fflush(pOutput);
	}
}

AVFrame * RecorderManager::creatFrame()
{
	AVFrame *pFrame = av_frame_alloc();
	if (pFrame) {
		// 单通道一个音频帧的采样数
		pFrame->nb_samples = 512;
		// 采样大小
		pFrame->format = AV_SAMPLE_FMT_S16;
		// channel layout
		pFrame->channel_layout = AV_CH_LAYOUT_STEREO;
		av_frame_get_buffer(pFrame, 0);
		if (pFrame->buf[0]) {
			return pFrame;
		}
	}
	return NULL;
}

SwrContext * RecorderManager::creatSwrContext()
{
	//创建重采样对象
	SwrContext *pSwrCtx = NULL;
	pSwrCtx = swr_alloc_set_opts(NULL,//重采样上下文
		AV_CH_LAYOUT_STEREO,  //输出的layout, 如：5.1声道
		AV_SAMPLE_FMT_S16,    //输出的采样格式 Float, S16, S24
		44100,                 //输出采样率
		AV_CH_LAYOUT_STEREO,  //输入的layout
		AV_SAMPLE_FMT_S16,   //输入的样本格式
		44100,				//输入的采样率
		0, NULL);			//任意NULL
	// 初始化上下文
	auto ret = swr_init(pSwrCtx);
	if (ret < 0) {
		qInfo() << "swr_init fail" << ret;
		return NULL;
	}
	return pSwrCtx;
}

void RecorderManager::alloc_data_resample(uint8_t *** src_data, int * src_linesize, uint8_t *** dst_data, int * dst_linesize)
{
	//创建输入缓冲区
	av_samples_alloc_array_and_samples(
		src_data,			//输入缓冲区地址
		src_linesize,     //缓冲区大小
		2,                 //通道个数
		512,               //单通道采样个数
		AV_SAMPLE_FMT_S16,
		0);

	//创建输出缓冲区
	av_samples_alloc_array_and_samples(
		dst_data,         //输出缓冲区地址
		dst_linesize,     //缓冲区大小
		2,                 //通道个数
		512,                 //单通道采样个数
		AV_SAMPLE_FMT_S16,//采样格式
		0);
}
