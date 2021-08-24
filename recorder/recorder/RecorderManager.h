#pragma once

#include <QObject>

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavfilter/avfilter.h>
	#include <libavdevice/avdevice.h>
	#include <libswresample/swresample.h>
}

class RecorderManager : public QObject
{
	Q_OBJECT
public:
	RecorderManager(QObject *parent = Q_NULLPTR);
	~RecorderManager();

	//�޸�¼��״̬
	void changeRecorderState(bool state);

	//��ʼ��Ƶ¼��
	void startRecorderAudio();

private:
	//���豸
	AVFormatContext *openDev();
	//����������
	AVCodecContext *openEncoder();
	//���ݴ���
	void encode(AVCodecContext *pCtx, AVFrame *pFrame, AVPacket *pkt, FILE* pOutput);
	//��Ƶ��������
	AVFrame *creatFrame();
	//�����ز���������
	SwrContext* creatSwrContext();
	//����������
	void alloc_data_resample(uint8_t ***src_data, int *src_linesize, uint8_t ***dst_data, int *dst_linesize);

private:
	bool m_isStop;
};
