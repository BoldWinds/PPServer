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

    //发送消息的connet函数
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

//设置信息
void ServerSingleton::getNetworkInfo(){
    QHostInfo::lookupHost(hostinfo.localHostName(), this, SLOT(slotGetAddress(QHostInfo)));
    hostaddr.setAddress(serverIP);
    qDebug() << hostaddr;
}

void ServerSingleton::openServer(QString ip,QString port){
    if(this->isListening()){
        //emit sig_update_gui("Server is already listening!");
    }else if(this->listen(QHostAddress(ip), port.toUInt())){
        qDebug() << "Server listening...";
        qDebug() << "Server ip is" << ip;
        qDebug() << "Server port is " + port;
        //emit sig_update_gui("Server starts listening.");
        //emit sig_update_gui("Server ip  : " + ip);
        //emit sig_update_gui("Server port: " + port);
    }
}

void ServerSingleton::openServer(){
    //默认在本地9000端口开启
    openServer("127.0.0.1","9000");
}

void ServerSingleton::closeServer(){
    this->close();
    //对online表中的每一个用户进行下线处理
    for(auto user:onlineSet){
        //emit signalOffline(user);
        onlineSet.remove(user);
        qDebug()<<"user: "<<user<<"offline";
    }
    qDebug()<<"SERVER CLOSED!";
}

void ServerSingleton::closeSocket(qintptr descriptor){
    //hash中不含该标识符对应的socket
    if(socketHash.find(descriptor) == socketHash.end()){
        qDebug()<<"NO SOCKET "<<descriptor;
    }else{
        ServerSocketThread* serverSocketThread = socketHash[descriptor];

        serverSocketThread->close();
        serverSocketThread->quit();
        serverSocketThread->deleteLater();

        socketHash.remove(descriptor);
    }

}

void ServerSingleton::closeSocket(QString userID){
    closeSocket(descriptorHash[userID]);
}

QString ServerSingleton::getNickname(QString userID){
    //若hash里没有则去数据库里查找并加入到hash里


    return nicknameHash[userID];
}

void ServerSingleton::slotSendMessage(QString userID, const QByteArray message){
    qDebug() << "Sending message: " << message;

    qintptr descriptor = descriptorHash[userID];
    ServerSocketThread* serverSocketThread = socketHash[descriptor];
    serverSocketThread->write(message);
}

void ServerSingleton::slotSendMessage(qintptr descriptor, const QByteArray message){
       qDebug() << "Sending message: " << message;

       ServerSocketThread* serverSocketThread = socketHash[descriptor];
       serverSocketThread->write(message);
}

void ServerSingleton::slotGetAddress(QHostInfo hostInfo){
    //emit signalGetIPList(hostInfo);
}


void ServerSingleton::newConnection(qintptr descriptor){

    ServerSocketThread *serverSocketThread = nullptr;




    //断开连接处理
    connect(serverSocketThread, &ServerSocketThread::signalDisconnectedDescriptor, [this](qintptr des){
        closeSocket(des);
    });
    connect(serverSocketThread, &ServerSocketThread::signalDisconnectedUserID, [this](QString userID){
        closeSocket(userID);
    });
}

