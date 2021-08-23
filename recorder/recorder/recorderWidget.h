#pragma once

#include <QtWidgets/QWidget>
#include "ui_recorderWidget.h"

class recorder : public QWidget
{
    Q_OBJECT

public:
    recorder(QWidget *parent = Q_NULLPTR);
	~recorder();

private:
    Ui::recorderClass ui;
};
