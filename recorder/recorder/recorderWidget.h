#pragma once

#include <QtWidgets/QWidget>
#include <QSharedPointer>
#include "ui_recorderWidget.h"

class RecorderManager;
class QThread;
class recorderWindow;

class recorder : public QWidget
{
    Q_OBJECT

public:
    recorder(QWidget *parent = Q_NULLPTR);
	~recorder();

private slots:
	void sltRecorderStart();
	void sltRecorderStop();

	void sltRecorderWindow();

private:
    Ui::recorderClass ui;
	//¼�ƶ���
	RecorderManager *m_recorder = nullptr;
	//¼���߳�
	QThread *m_thread = nullptr;

	//��Ļ��׽
	QSharedPointer<recorderWindow> m_recorderWindow = Q_NULLPTR;
};
