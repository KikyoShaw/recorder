#include "audioRecorder.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avio.h"
#include "libavutil/imgutils.h"
}

#include <QDebug>
#include <QFile>

int recorderAudio()
{
	//初始化所有设备
	avdevice_register_all();
	//分配format上下文
	AVFormatContext *formatContext = avformat_alloc_context();
	//dshow可以用来抓取摄像头、采集卡、麦克风等，
	//vfwcap主要用来采集摄像头类设备，
	//gdigrab则是抓取Windows窗口程序。 
	//avfoundation-MAC/dshow-windosw/alsa-linux 其他系统用到的接口
	AVInputFormat *inputFormat = av_find_input_format("dshow");
	//设备命名规则：audio=当前电脑录音设备名
	QString audioInputName = QStringLiteral("audio=VB-Audio Cable A");
	//打开设备
	if (avformat_open_input(&formatContext, audioInputName.toUtf8().data(), inputFormat, nullptr) != 0) {
		qDebug() << "can`t open input device";
		return -1;
	}
	//寻找音频流
	int audioIndex = -1;
	for (uint i = 0; i < formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioIndex = i;
			break;
		}
	}

	AVCodecParameters *codecParamter = formatContext->streams[audioIndex]->codecpar;
	AVCodec* codec = avcodec_find_decoder(codecParamter->codec_id);
	AVCodecContext* codecContext = avcodec_alloc_context3(codec);
	avcodec_parameters_to_context(codecContext, codecParamter);
	avcodec_open2(codecContext, codec, nullptr);

	AVFrame *frame = av_frame_alloc();
	AVPacket *packet = av_packet_alloc();
	QFile file("output2.pcm");
	if (!file.open(QFile::WriteOnly)) {
		qDebug() << "can`t open file";
		return -1;
	}

	qDebug() << "audio info:" << endl
		<< codecParamter->format << codecParamter->bit_rate << codecParamter->sample_rate << codecParamter->channels;

	float time = 0;
	while (time <= 10.0) {
		if (av_read_frame(formatContext, packet) != 0) {
			qDebug() << "can`t read frame";
			return -1;
		}
		if (packet->stream_index == audioIndex) {
			if (avcodec_send_packet(codecContext, packet) != 0) {
				continue;
			}
			if (avcodec_receive_frame(codecContext, frame) != 0) {
				continue;
			}

			int pcmSize = av_samples_get_buffer_size(nullptr, codecParamter->channels, frame->nb_samples, codecContext->sample_fmt, 1);
			float usedTime = frame->nb_samples*1.0 / codecParamter->sample_rate;
			time += usedTime;
			qDebug() << pcmSize << usedTime << time;
			file.write((char*)frame->data[0], pcmSize);
			av_packet_unref(packet);
		}
	}
	av_frame_free(&frame);
	av_packet_free(&packet);
	avformat_close_input(&formatContext);
	avcodec_free_context(&codecContext);
	avformat_free_context(formatContext);
	file.close();
	return 0;
}
