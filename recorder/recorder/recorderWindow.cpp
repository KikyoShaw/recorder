#include "recorderWindow.h"
#include <QDebug>
#include <QPainter>
#include <QTimer>

recorderWindow::recorderWindow(QWidget *parent)
	: QWidget(parent)
{
	//初始化ffmpeg设备
	initDevice();
	//定时器刷新刷新
	QTimer *timer = new QTimer;     
	if (timer) {
		timer->setTimerType(Qt::PreciseTimer);
		timer->setInterval(20);
		timer->start();
		connect(timer, &QTimer::timeout, this, static_cast<void (QWidget::*)()>(&QWidget::repaint));
	}
	//刷新界面
	resize(codecParameters->width*0.6, codecParameters->height*0.6);
}

recorderWindow::~recorderWindow()
{
}

void recorderWindow::initDevice()
{
	//初始化所有设备
	avdevice_register_all();                                    
	 //分配format上下文
	formatContext = avformat_alloc_context();                    
	//寻找输入设备【gdigrab】
	AVInputFormat *inputFormat = av_find_input_format("gdigrab"); 
	AVDictionary* options = NULL;
	//设置帧数为60
	av_dict_set(&options, "framerate", "60", 0);                   
	//开启输入设备
	if (avformat_open_input(&formatContext, "desktop", inputFormat, &options)) { 
		qDebug() << "cant`t open input stream.";
		return;
	}
	//加载流中存储的信息
	if (avformat_find_stream_info(formatContext, nullptr)) {       
		qDebug() << "can`t find stream information.";
		return;
	}
	//寻找视频流
	videoIndex = -1;                                             
	for (uint i = 0; i < formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoIndex = i;
			break;
		}
	}
	if (videoIndex == -1) {
		qDebug() << "can`t find video stream.";
		return;
	}
	codecParameters = formatContext->streams[videoIndex]->codecpar;
	codecContext = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(codecContext, codecParameters);
	AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);

	if (codec == nullptr) {
		qDebug() << "can`t find codec";
		return;
	}
	if (avcodec_open2(codecContext, codec, nullptr)) {
		qDebug() << "can`t open codec";
		return;
	}

	packet = av_packet_alloc();
	frame = av_frame_alloc();

	imgConvertContext = sws_getContext(codecParameters->width, codecParameters->height, codecContext->pix_fmt, 
		codecParameters->width, codecParameters->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
	image = QImage(codecParameters->width, codecParameters->height, QImage::Format_RGB888);
	av_image_fill_linesizes(lineSize, AV_PIX_FMT_RGB24, codecParameters->width);
}

void recorderWindow::paintEvent(QPaintEvent *event)
{
	if (av_read_frame(formatContext, packet)) {
		return;
	}
	if (packet->stream_index == videoIndex) {
		if (avcodec_send_packet(codecContext, packet))
			return;
		if (avcodec_receive_frame(codecContext, frame))
			return;
		uint8_t* dst[] = { image.bits() };
		sws_scale(imgConvertContext, frame->data, frame->linesize, 0, codecParameters->height, dst, lineSize);
		//清空数据包
		av_packet_unref(packet);   
		//绘制
		QPainter painter(this);
		painter.fillRect(rect(), image.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
	}
}

void recorderWindow::closeEvent(QCloseEvent *event)
{
	emit sigRecorderWindowClose();
	QWidget::closeEvent(event);
}
