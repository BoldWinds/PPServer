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
        qDebug()<<"server has already been listening";
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
        emit signalOffline(user);
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
    if(nicknameHash.find(userID) == nicknameHash.end()){
        sqlManipulation* sql = sqlManipulation::instantiation();
        nicknameHash.insert(userID,sql->get_nickname(userID));
    }
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
    emit signalGetIPList(hostInfo);
}


void ServerSingleton::incomingConnection(qintptr descriptor){

    qDebug()<<"NEW CONNECTION!";
    ServerSocketThread *serverSocketThread = nullptr;

    //新建socket并分配标识符
    if(socketHash.find(descriptor) == socketHash.end()){
        //当前hash里没有该描述符对应的socket
        serverSocketThread = new ServerSocketThread();
        serverSocketThread->start();
        serverSocketThread->recordDescriptor(descriptor);
        socketHash.insert(descriptor,serverSocketThread);
    }else{
        //当前hash里已经有该描述符所对应的socket
        serverSocketThread = socketHash[descriptor];
        serverSocketThread->recordDescriptor(descriptor);
    }

    qDebug()<<"NEW CONNECTION HAS BEEN ESTABLISHED!";

    //具体业务逻辑处理
    connect(serverSocketThread,SIGNAL(signalReadyRead(qintptr,QByteArray)),
                this,SLOT(slotReadMessage(qintptr,QByteArray)));


    //断开连接处理
    connect(serverSocketThread, &ServerSocketThread::signalDisconnectedDescriptor, [this](qintptr des){
        closeSocket(des);
    });
    connect(serverSocketThread, &ServerSocketThread::signalDisconnectedUserID, [this](QString userID){
        closeSocket(userID);
    });
}

void ServerSingleton::slotReadMessage(qintptr descriptor, QByteArray message){
    QDataStream messageStream(&message, QIODevice::ReadOnly);
    QByteArray header;
    messageStream >> header;
    qDebug() << "Header : " << header;

    if(header.startsWith("REGISTER")){

    }else if(header.startsWith("LOGIN")){

    }else if(header.startsWith("GET_FRIENDS")){

    }else if(header.startsWith("ADD_FRIEND")){

    }else if(header.startsWith("ADD_FRIEND_SUCCESS")){

    }else if(header.startsWith("ADD_FRIEND_FAIL")){

    }else if(header.startsWith("CREATE_GROUP")){

    }else if(header.startsWith("PRIVATE_CHAT")){

    }else if(header.startsWith("PUBLIC_CHAT")){

    }else if(header.startsWith("ADD_GROUP")){

    }else if(header.startsWith("ADD_GROUP_SUCCESS")){

    }else if(header.startsWith("ADD_GROUP_FAIL")){

    }else if(header.startsWith("INVITE_GROUP")){

    }else if(header.startsWith("INVITE_GROUP_SUCCESS")){

    }else if(header.startsWith("INVITE_GROUP_FAIL")){

    }else if(header.startsWith("SEND_FILE")){

    }
}

