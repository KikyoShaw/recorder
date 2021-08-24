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

	//修改录制状态
	void changeRecorderState(bool state);

	//开始音频录制
	void startRecorderAudio();

private:
	//打开设备
	AVFormatContext *openDev();
	//创建编码器
	AVCodecContext *openEncoder();
	//数据处理
	void encode(AVCodecContext *pCtx, AVFrame *pFrame, AVPacket *pkt, FILE* pOutput);
	//音频输入数据
	AVFrame *creatFrame();
	//创建重采样上下文
	SwrContext* creatSwrContext();
	//缓存区操作
	void alloc_data_resample(uint8_t ***src_data, int *src_linesize, uint8_t ***dst_data, int *dst_linesize);

private:
	bool m_isStop;
};
