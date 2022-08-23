#include "serversocket.h"

void ServerSocket::recordDescriptor(qintptr _des){
    descriptor = _des;
    setSocketDescriptor(_des);
}

void ServerSocket::recordUserID(QString _userID){
    userID = _userID;
}

ServerSocket::ServerSocket(QObject *parent) : QTcpSocket(parent){

    connect(this,&ServerSocket::readyRead,this,[this](){
        QByteArray message;
        message = readAll();
        emit signalReadyRead(socketDescriptor(),message);
    });

    connect(this,&ServerSocket::disconnected ,this,[this](){
        if(state() != QAbstractSocket::UnconnectedState){
            return;
        }

        qDebug()<<"socket "<<descriptor<<" has disconnected!";

        if(QString::localeAwareCompare(userID, QString("")) == 0){
            qDebug()<<"HAVE NOT SIGNED UP";
            emit signalDisconnectedDescriptor(descriptor);
        }else{
            qDebug()<<"user: "<<userID<<"disconnected";
            emit signalDisconnectedUserID(userID);
        }
    });
}
