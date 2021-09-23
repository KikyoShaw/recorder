#pragma once

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avio.h"
#include "libavutil/imgutils.h"
}

#include <QWidget>

class recorderWindow : public QWidget
{
	Q_OBJECT
public:
	recorderWindow(QWidget *parent = nullptr);
	~recorderWindow();

private:
	void initDevice();

signals:
	void sigRecorderWindowClose();

protected:
	virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
	virtual void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
	AVFormatContext *formatContext = NULL;
	AVCodecParameters *codecParameters = NULL;
	int videoIndex;
	AVCodecContext* codecContext = NULL;
	AVPacket *packet = NULL;
	AVFrame *frame = NULL;
	SwsContext *imgConvertContext = NULL;
	QImage image;
	int lineSize[4];
};
