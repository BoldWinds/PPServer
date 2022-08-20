#include "serversocket.h"

void ServerSocket::record_descriptor(qintptr _des){
    descriptor = _des;
    setSocketDescriptor(_des);
}

void ServerSocket::record_userID(QString _userID){
    userID = _userID;
}

ServerSocket::ServerSocket(QObject *parent) : QTcpSocket(parent){

    connect(this,&ServerSocket::readyRead,this,[this](){
        QByteArray message;
        message = readAll();
        emit signal_readyRead(socketDescriptor(),message);
    });

    connect(this,&ServerSocket::disconnected ,this,[this](){
        if(state() != QAbstractSocket::UnconnectedState){
            return;
        }

        qDebug()<<userID;

        if(QString::localeAwareCompare(userID, QString("")) == 0){
            qDebug()<<"HAVE NOT SIGNED UP";
            emit signal_disconnected_descriptor(descriptor);
        }else{
            qDebug()<<"socket";
            emit signal_disconnected_userID(userID);
        }
    });
}
