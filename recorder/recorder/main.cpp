#include "recorderWidget.h"
#include <QtWidgets/QApplication>

#include "mainwindow.h"	

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //recorder w;
	MainWindow w;
    w.show();
    return a.exec();
}
