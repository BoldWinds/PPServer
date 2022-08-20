#include <QDataStream>
#include <future>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include "serversingleton.h"

ServerSingleton* ServerSingleton::instance = nullptr;

ServerSingleton::ServerSingleton(QObject *parent) : QTcpServer(parent)
{
    getNetworkInfo();

    connect(this, SIGNAL(signalSendMessage(QString, const QByteArray)),
            this, SLOT(slotSendMessage(QString, const QByteArray)));

    //这段代码？？
    if(!QMetaType::isRegistered((QMetaType::type("qintptr")))){
        qRegisterMetaType<qintptr>("qintptr");
    }

    connect(this, SIGNAL(signalSendMessage(qintptr, const QByteArray)),
            this, SLOT(slotSendMessage(qintptr, const QByteArray)));
    //    TcpServerSingleton::qtid_distributed = INIT_QTID + ServerSqlSingleton::account_number;
   // heart_timer = startTimer(10000);
}


ServerSingleton* ServerSingleton::getInstance(){
    if(ServerSingleton::instance == nullptr){
        QMutex mutex;
        //使用锁防止确保只有一个单例
        mutex.lock();
        ServerSingleton::instance = new ServerSingleton();
        mutex.unlock();
    }
    return ServerSingleton::instance;
}

void ServerSingleton::getNetworkInfo(){

}

void ServerSingleton::openServer(QString ip,QString port){

}

void ServerSingleton::openServer(){
    //默认在本地9000端口开启
    openServer("127.0.0.1","9000");
}

void ServerSingleton::closeServer(){

}

void ServerSingleton::closeSocket(qintptr descriptor){

}

void ServerSingleton::closeSocket(QString userID){
    closeSocket(descriptorHash.value(userID));
}

void ServerSingleton::newConnection(qintptr descriptor){

}


void ServerSingleton::slotSendMessage(QString userID, const QByteArray message){
    qDebug() << "Sending message: " << message;
//    QtConcurrent::run(QThreadPool::globalInstance(), [this](qintptr des, QByteArray message){
    qintptr descriptor = descriptorHash[userID];
    ServerSocketThread* serverSocketThread = socketHash[descriptor];
    serverSocketThread->write(message);
//    },des, message);
}

void ServerSingleton::slotSendMessage(qintptr descriptor, const QByteArray message){
       qDebug() << "Sending message: " << message;
       ServerSocketThread* serverSocketThread = socketHash[descriptor];
       serverSocketThread->write(message);
}

void ServerSingleton::slotGetAddress(QHostInfo hostInfo){

}


