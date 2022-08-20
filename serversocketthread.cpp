#include "serversocketthread.h"
#include "serversocket.h"

ServerSocketThread::ServerSocketThread(QObject *parent, qintptr descriptor) : QThread(parent)
{
    serverSocket = new ServerSocket();
    serverSocket->setSocketDescriptor(descriptor);
    serverSocket->record_descriptor(descriptor);
}

QByteArray ServerSocketThread::read(){
    return serverSocket->readAll();
}

void ServerSocketThread::write(QByteArray message){
    //若socket是连接状态则直接发送
    if(serverSocket->state() == QAbstractSocket::ConnectedState){
        serverSocket->write(message);
        qDebug()<<"SEND MESSAGE";
        return;
    }
    qDebug()<<"MESSAGE SEND FAILURE";
}

void ServerSocketThread::record_userID(QString _userID)
{
    qDebug() << _userID;
    userID = _userID;
    //同时更新线程所绑定socket的userID
    serverSocket->record_userID(_userID);
}

void ServerSocketThread::record_descriptor(qintptr _descriptor)
{
    descriptor = _descriptor;
    //同时更新线程所绑定socket的descriptor
    serverSocket->record_descriptor(_descriptor);
}

void ServerSocketThread::close(){
    //停止循环
    checkpoint = false;
}

void ServerSocketThread::run(){

    connect(serverSocket, &ServerSocket::signal_disconnected_descriptor, this, &ServerSocketThread::slot_disconnected_descriptor);
    connect(serverSocket, &ServerSocket::signal_disconnected_userID, this, &ServerSocketThread::slot_disconnected_userID);
    connect(serverSocket, &ServerSocket::signal_readyRead, this, &ServerSocketThread::slot_readyRead);

    if(serverSocket->waitForConnected()){
        qDebug()<<"connected";
    }

    while(serverSocket->state() == QAbstractSocket::ConnectedState and checkpoint){
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, SLOT(quit()));
        loop.exec();
    }
}

void ServerSocketThread::slot_disconnected_userID(QString _userID)
{
    emit ServerSocketThread::signal_disconnected_userID(userID);
}

void ServerSocketThread::slot_disconnected_descriptor(qintptr _descriptor)
{
    emit ServerSocketThread::signal_disconnected_descriptor(descriptor);
}

void ServerSocketThread::slot_readyRead(qintptr _descriptor, QByteArray message)
{
    emit ServerSocketThread::signal_readyRead(_descriptor, message);
}

QAbstractSocket::SocketState ServerSocketThread::state(){
    return serverSocket->state();
}
