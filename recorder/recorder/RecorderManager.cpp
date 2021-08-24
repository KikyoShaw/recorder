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
	//����·��
	QString path = "test.aac";

	FILE *outfile = fopen(path.toLatin1().data(), "wb+");

	//��ʼ��paket
	int count = 0;
	AVPacket packet;
	av_init_packet(&packet);

	//�ز���������
	uint8_t **src_data = NULL;
	int src_linesize = 0;
	uint8_t **dst_data = NULL;
	int dst_linesize = 0;

	//���豸
	AVFormatContext* pFormatCtx = openDev();
	if (NULL == pFormatCtx)
		return;
	//�򿪱�����������
	AVCodecContext* pCodecCtx = openEncoder();
	if (NULL == pCodecCtx)
		return;
	//��Ƶ��������
	//����������������
	AVFrame *pFrame = creatFrame();
	if (NULL == pFrame)
		return;
	//������������ݿռ�
	AVPacket *pNewpkt = av_packet_alloc();
	if (NULL == pNewpkt)
		return;
	//�����ز���������
	SwrContext* pSwrCtx = creatSwrContext();
	if (NULL == pNewpkt)
		return;
	//��Ҫ��ʱ�������ɼ�һ����������֮���ٸ����������б��룬��Ϊ��ͨ��������С����Ϊ32��pktsizeΪ64 64/2(2ͨ��)/2(16bit)<32
	int swr_num = 0;
	uint8_t *bufferData = new uint8_t[2048];
	alloc_data_resample(&src_data, &src_linesize, &dst_data, &dst_linesize);//�����ز���������

	auto ret = av_read_frame(pFormatCtx, &packet);
	while ((ret == 0) && m_isStop == false)// ���豸��ȡ����
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
			qInfo() << QStringLiteral("�ز������뻺������С =") << src_linesize;
			memcpy((void*)src_data[0], (void *)bufferData, 2048);//��ȡ�豸��Ƶ���ݻ������������ز������뻺����

			swr_convert(pSwrCtx,
				dst_data,
				512,
				(const uint8_t **)src_data,
				512);
			qInfo() << QStringLiteral("�ز��������������С =") << dst_linesize;
			memcpy((void *)pFrame->data[0], (void *)dst_data[0], dst_linesize);//�ز��������������������������뻺����

			encode(pCodecCtx, pFrame, pNewpkt, outfile);
		}
		// �ͷ�packet�ռ�
		av_packet_unref(&packet);
	}
	encode(pCodecCtx, NULL, pNewpkt, outfile);
	// �ͷ�AVFrame �� newpkt
	if (pFrame) {
		av_frame_free(&pFrame);
	}
	if (pNewpkt) {
		av_packet_free(&pNewpkt);
	}
	//�ͷ����������������Դ
	if (src_data) {
		av_freep(&src_data[0]);
	}
	av_freep(&src_data);

	if (dst_data) {
		av_freep(&dst_data[0]);
	}
	av_freep(&dst_data);
	//�ͷ��ز���������
	if (pSwrCtx) {
		swr_free(&pSwrCtx);
	}
	//�ͷű�����������
	if (pCodecCtx) {
		av_free(pCodecCtx);
	}
	//�ͷ��豸
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
	//�����ļ�������
	AVFormatContext *pFormatCtx = NULL/*avformat_alloc_context()*/;
	AVInputFormat *pFormatInput = av_find_input_format("alsa");
	char *deviceName = "hw:0";

	//AVDictionary* opts = NULL;
	////����tcp or udp��Ĭ��һ������tcp�ٳ���udp
	//av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	////���ó�ʱ3��
	//av_dict_set(&opts, "stimeout", "60000000", 0);

	//������Ƶ
	auto ret = avformat_open_input(&pFormatCtx, deviceName, pFormatInput, NULL/*&opts*/);
	if (ret < 0){
		//��ӡ����
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
	//���ձ��������ƴ���������
	AVCodec *codec = avcodec_find_encoder_by_name("libfdk_aac");
	//����codec������
	if (codec) {
		//����AVCodecContext�ռ�
		AVCodecContext *pCodecCtx = avcodec_alloc_context3(codec);
		// ������Ƶ�Ĳ�����С��fdk_aac��Ҫ16λ����Ƶ��������	
		pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
		// ������Ƶ��CHANNEL LAYOUT
		pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
		// ������Ƶ��������
		pCodecCtx->channels = 2;
		// ������Ƶ�Ĳ�����
		pCodecCtx->sample_rate = 44100;
		// AAC : 128K   AAV_HE: 64K  AAC_HE_V2: 32K. bit_rateΪ0ʱ�Ż����profile����ֵ
		pCodecCtx->bit_rate = 0;
		//profile����ֵ
		pCodecCtx->profile = FF_PROFILE_AAC_HE;
		// �򿪱�����
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
	//�������ݵ�������
	auto ret = avcodec_send_frame(pCtx, pFrame);
	if (ret < 0) {
		qInfo() << "avcodec_send_frame fail: " << ret;
		return;
	}
	//���ݱ���д���ļ�
	while (ret >= 0){
		// ��ȡ��������Ƶ����
		ret = avcodec_receive_packet(pCtx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
			return;
		}
		else if (ret < 0){
			qInfo() << "avcodec_receive_packet fail";
			exit(-1);
		}
		// д���ļ�
		fwrite(pkt->data, 1, pkt->size, pOutput);
		fflush(pOutput);
	}
}

AVFrame * RecorderManager::creatFrame()
{
	AVFrame *pFrame = av_frame_alloc();
	if (pFrame) {
		// ��ͨ��һ����Ƶ֡�Ĳ�����
		pFrame->nb_samples = 512;
		// ������С
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
	//�����ز�������
	SwrContext *pSwrCtx = NULL;
	pSwrCtx = swr_alloc_set_opts(NULL,//�ز���������
		AV_CH_LAYOUT_STEREO,  //�����layout, �磺5.1����
		AV_SAMPLE_FMT_S16,    //����Ĳ�����ʽ Float, S16, S24
		44100,                 //���������
		AV_CH_LAYOUT_STEREO,  //�����layout
		AV_SAMPLE_FMT_S16,   //�����������ʽ
		44100,				//����Ĳ�����
		0, NULL);			//����NULL
	// ��ʼ��������
	auto ret = swr_init(pSwrCtx);
	if (ret < 0) {
		qInfo() << "swr_init fail" << ret;
		return NULL;
	}
	return pSwrCtx;
}

void RecorderManager::alloc_data_resample(uint8_t *** src_data, int * src_linesize, uint8_t *** dst_data, int * dst_linesize)
{
	//�������뻺����
	av_samples_alloc_array_and_samples(
		src_data,			//���뻺������ַ
		src_linesize,     //��������С
		2,                 //ͨ������
		512,               //��ͨ����������
		AV_SAMPLE_FMT_S16,
		0);

	//�������������
	av_samples_alloc_array_and_samples(
		dst_data,         //�����������ַ
		dst_linesize,     //��������С
		2,                 //ͨ������
		512,                 //��ͨ����������
		AV_SAMPLE_FMT_S16,//������ʽ
		0);
}
