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
	//¼�ƶ���
	RecorderManager *m_recorder = nullptr;
	//¼���߳�
	QThread *m_thread = nullptr;
};
