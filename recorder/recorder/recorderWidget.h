#pragma once

#include <QtWidgets/QWidget>
#include "ui_recorderWidget.h"

class RecorderManager;
class QThread;

class recorder : public QWidget
{
    Q_OBJECT

public:
    recorder(QWidget *parent = Q_NULLPTR);
	~recorder();

private slots:
	void sltRecorderStart();
	void sltRecorderStop();

private:
    Ui::recorderClass ui;
	//录制对象
	RecorderManager *m_recorder = nullptr;
	//录制线程
	QThread *m_thread = nullptr;
};
