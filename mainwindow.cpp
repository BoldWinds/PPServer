#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ServerSingleton* server_instance=ServerSingleton::getInstance();

    connect(ui->open_server,&QPushButton::clicked,this,&MainWindow::open_server);
    connect(ui->close_server,&QPushButton::clicked,this,&MainWindow::close_server);
    connect(server_instance,&ServerSingleton::signalOnline,this,&MainWindow::online_increase);
    connect(server_instance,&ServerSingleton::signalOffline,this,&MainWindow::online_decrease);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::open_server(){
    qDebug()<<"open server";
    ServerSingleton::getInstance()->openServer();
}

void MainWindow::close_server(){
    ServerSingleton::getInstance()->closeServer();

}

void MainWindow::online_increase(QString userID){
    qDebug()<<"-------------------------";
     ui->user_list->addItem(QString("User:%1").arg(userID));
}

void MainWindow::online_decrease(QString userID){
    for (int i=0;i<ui->user_list->count();i++) {
        QString str=ui->user_list->item(i)->text().mid(5);
        if (str.compare(userID)==0) {
            QListWidgetItem* item = ui->user_list->takeItem(i);
            delete item;
        }
    }
}
