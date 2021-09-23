#include "recorderWidget.h"
#include "RecorderManager.h"
#include <QThread>
#include "recorderWindow.h"

recorder::recorder(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);

	//初始化录制线程
	m_recorder = new RecorderManager(this);
	if (m_recorder) {
		m_thread = new QThread(this);
		if (m_thread) {
			m_recorder->moveToThread(m_thread);
			m_thread->start();
		}
	}

	connect(ui.pushButton_start, &QPushButton::clicked, this, &recorder::sltRecorderStart);
	connect(ui.pushButton_stop, &QPushButton::clicked, this, &recorder::sltRecorderStop);
	connect(ui.pushButton_start_2, &QPushButton::clicked, this, &recorder::sltRecorderWindow);
}

recorder::~recorder()
{
	if (m_thread) {
		m_thread->quit();
		m_thread->wait();
	}
}

void recorder::sltRecorderStop()
{
	if (m_thread) {
		if (true == m_thread->isRunning()) {
			if (m_recorder) {
				m_recorder->changeRecorderState(true);
			}
			m_thread->quit();
			m_thread->wait();
		}
	}
}

void recorder::sltRecorderWindow()
{
	m_recorderWindow = QSharedPointer<recorderWindow>(new recorderWindow());
	if (m_recorderWindow) {
		connect(m_recorderWindow.data(), &recorderWindow::sigRecorderWindowClose, this, &recorder::show);
		this->hide();
		m_recorderWindow->show();
		m_recorderWindow->activateWindow();
	}
}

void recorder::sltRecorderStart()
{
	if (m_thread) {
		if (m_recorder) {
			m_recorder->changeRecorderState(false);
			m_recorder->startRecorderAudio();
		}
	}
}
