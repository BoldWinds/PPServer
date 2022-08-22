#include "mainwindow.h"
#include <QApplication>
#include "serversingleton.h"
#include "sqlmanipulation.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();


    ServerSingleton::getInstance()->openServer();
    sqlManipulation::instantiation();

    return a.exec();
}
