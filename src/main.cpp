#include <QApplication>
#include "MainWindow.h"
int main(int argc,char**argv){QApplication app(argc,argv);QApplication::setApplicationName("Freeway Simulator");MainWindow w;w.showMaximized();return app.exec();}
