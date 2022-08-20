#include "serversocketthread.h"
#include "serversocket.h"

ServerSocketThread::ServerSocketThread(QObject *parent, qintptr descriptor) : QThread(parent)
{
    serverSocket = new ServerSocket();
    serverSocket->setSocketDescriptor(descriptor);
    serverSocket->recordDescriptor(descriptor);
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

void ServerSocketThread::recordUserID(QString _userID)
{
    qDebug() << _userID;
    userID = _userID;
    //同时更新线程所绑定socket的userID
    serverSocket->recordUserID(_userID);
}

void ServerSocketThread::recordDescriptor(qintptr _descriptor)
{
    descriptor = _descriptor;
    //同时更新线程所绑定socket的descriptor
    serverSocket->recordDescriptor(_descriptor);
}

void ServerSocketThread::close(){
    //停止循环
    checkpoint = false;
}

void ServerSocketThread::run(){

    //处理该线程绑定的socket所发出来的信号
    connect(serverSocket, &ServerSocket::signalDisconnectedDescriptor, this, &ServerSocketThread::slotDisconnectedDescriptor);
    connect(serverSocket, &ServerSocket::signalDisconnectedUserID, this, &ServerSocketThread::slotDisconnectedUserID);
    connect(serverSocket, &ServerSocket::signalReadyRead, this, &ServerSocketThread::slotReadyRead);

    if(serverSocket->waitForConnected()){
        qDebug()<<"connected";
    }

    while(serverSocket->state() == QAbstractSocket::ConnectedState and checkpoint){
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, SLOT(quit()));
        loop.exec();
    }
}

void ServerSocketThread::slotDisconnectedUserID(QString _userID)
{
    //该信号在singleton被捕获
    emit ServerSocketThread::signalDisconnectedUserID(_userID);
}

void ServerSocketThread::slotDisconnectedDescriptor(qintptr _descriptor)
{
    //该信号在singleton被捕获
    emit ServerSocketThread::signalDisconnectedDescriptor(_descriptor);
}

void ServerSocketThread::slotReadyRead(qintptr _descriptor, QByteArray message)
{
    //该信号在singleton被捕获
    emit ServerSocketThread::signalReadyRead(_descriptor, message);
}

QAbstractSocket::SocketState ServerSocketThread::state(){
    return serverSocket->state();
}
