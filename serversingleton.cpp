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

    connect(this,SIGNAL(signalOnline(QString)),
                        this,SLOT(slotUserOnline(QString)));

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

        qDebug()<<"socket: "<<descriptor<<" closed";
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
    if(onlineSet.find(userID) == onlineSet.constEnd()){
        //这哥们不在线
        qDebug()<<"receiver offline";
        if(offlinemessageHash.find(userID) == offlinemessageHash.end()){
            //目前没有该用户的未读消息
            QList<QByteArray> offlineMsgs = QList<QByteArray>();
            offlineMsgs.append(message);
            offlinemessageHash.insert(userID,offlineMsgs);
        }else{
            //已经有该用户的未读消息
            if(offlinemessageHash[userID].size()>=100){
                //缓存的离线消息过多，放弃旧的加入新的
                offlinemessageHash[userID].removeFirst();
            }
            offlinemessageHash[userID].append(message);
        }
    }else{
        qintptr descriptor = descriptorHash[userID];
        ServerSocketThread* serverSocketThread = socketHash[descriptor];
        serverSocketThread->write(message);
    }
}

void ServerSingleton::slotSendMessage(qintptr descriptor, const QByteArray message){
       qDebug() << "Sending message: " << message;

       ServerSocketThread* serverSocketThread = socketHash[descriptor];
       serverSocketThread->write(message);
}

void ServerSingleton::slotGetAddress(QHostInfo hostInfo){
    emit signalGetIPList(hostInfo);
}

void ServerSingleton::slotUserOnline(QString userID){
    //先检测有没有离线消息
    if(offlinemessageHash.find(userID) == offlinemessageHash.end()){
        //没有离线消息
        qDebug()<<"no offline message";
    }else{
        //有离线消息
        if(offlinemessageHash[userID].size()>0){
            for(auto message : offlinemessageHash[userID]){
                emit signalSendMessage(userID,message);
            }
            //删除发送的离线消息
            offlinemessageHash.remove(userID);
        }
    }
    onlineSet.insert(userID);


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
    QByteArray reply;
    QDataStream replyStream(&reply,QIODevice::WriteOnly);
    QString header;
    messageStream >> header;
    qDebug() << "Header : " << header;

    if(header.startsWith("REGISTER")){
        QString nickname,password,mail;
        messageStream >> nickname >> password >> mail;

        QString userID = sqlManipulation::instantiation()->register_account(nickname,password,mail);

        qDebug()<<"user: "<<nickname <<"'s userID is: "<<userID;

        QString header = "REGISTER_SUCCESS";
        replyStream<<header<<userID;

        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);

    }else if(header.startsWith("LOGIN")){
        QString userID,password;
        messageStream>>userID>>password;

        if(sqlManipulation::instantiation()->login_account(userID,password)){
            //登录成功
            QString header = "LOGIN_SUCCESS";
            replyStream<<header;

            QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
                emit signalSendMessage(descriptor,reply);
            },descriptor,reply);

            qDebug()<<"user: "<<userID<<" login";

            //离线消息发送
            if(offlinemessageHash.find(userID) == offlinemessageHash.end()){
                //没有离线消息
                qDebug()<<"no offline message";
            }else{
                //有离线消息
                if(offlinemessageHash[userID].size()>0){
                    for(auto message : offlinemessageHash[userID]){
                        QtConcurrent::run(QThreadPool::globalInstance(),[this](QString userID,QByteArray message){
                            emit signalSendMessage(userID,message);
                        },userID,message);
                    }
                    //删除发送的离线消息
                    offlinemessageHash.remove(userID);
                }
            }

            //Hash操作
            onlineSet.insert(userID);
            descriptorHash[userID] = descriptor;

            //给socket记录userID
            ServerSocketThread *serverSocketThread = socketHash[descriptor];
            serverSocketThread->recordUserID(userID);

        }else{
            QString header = "LOGIN_FAIL";
            replyStream<<header;
            emit signalSendMessage(descriptor,reply);
            qDebug()<<"user: "<<userID<<" login fail";
        }
    }else if(header.startsWith("RETRIEVE")){
        QString userID,mail,password;
        messageStream>> userID >> mail >> password;
        bool result;
        //检查邮箱是否正确
        bool mail_result=sqlManipulation::instantiation()->check_mail(userID,mail);
        if(mail_result){  //更新密码
            result=sqlManipulation::instantiation()->change_password(userID,password);
        }
        else result=false;

        if(result){
            QString header = "RETRIEVE_SUCCESS";
            replyStream<<header;
        }else{
            QString header = "RETRIEVE_FAIL";
            replyStream<<header;
        }
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);
    }else if(header.startsWith("GET_USER_INFO")){
        QString userID;
        messageStream >> userID;

        QString NickName,Mail;
        NickName=sqlManipulation::instantiation()->get_nickname(userID);
        Mail=sqlManipulation::instantiation()->get_mail(userID);

        QString header = "GET_USER_INFO_SUCCESS";
        replyStream << header << NickName << Mail;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);

    }else if(header.startsWith("GET_FRIENDS")){
        QString userID;
        messageStream >> userID;

        int friendsCount,groupsCount;

        sqlManipulation* sql = sqlManipulation::instantiation();

        QList<QString> friendIDs = sql->get_friendlist(userID);
        QList<QString> groupIDs = sql->get_grouplist(userID);

        friendsCount = friendIDs.size();
        groupsCount = groupIDs.count();

        QList<QString> friendnames = QList<QString>();
        QList<QString> groupnames = QList<QString>();

        for(auto friendID : friendIDs){
            friendnames.append(sql->get_nickname(friendID));
        }
        for(auto groupID : groupnames){
            groupnames.append(sql->get_groupName(groupID));
        }

        QString header = "GET_FRIENDS_SUCCESS";
        replyStream<<header<<friendsCount<<friendIDs<<friendnames
                  <<groupsCount<<groupIDs<<groupnames;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit signalSendMessage(descriptor,reply);
        },descriptor,reply);

    }else if(header.startsWith("UPDATE_USERINFO")){
        QString NickName,Mail,NewPassword,OriginalPassword;
        messageStream >> NickName>>Mail>>NewPassword>>OriginalPassword;
        QString userID=descriptorHash.key(descriptor);
        bool result=sqlManipulation::instantiation()->check_password(userID,OriginalPassword);
        if(result){
            sqlManipulation::instantiation()->change_nickname(userID,NickName);
            sqlManipulation::instantiation()->change_mail(userID,Mail);
            sqlManipulation::instantiation()->change_password(userID,NewPassword);
        }

        QString header = "UPDATE_USERINFO_SUCCESS";
        replyStream << header;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);
    }
    else if(header.startsWith("ADD_FRIEND")){
        QString sendUserID,sendUsername,receiverUserID;
        messageStream >> sendUserID>>sendUsername>>receiverUserID;

        QString header = "ADD_FRIEND";
        replyStream<<header<< sendUserID<<sendUsername<<receiverUserID;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](QString receiverUserID,QByteArray reply){
            emit signalSendMessage(receiverUserID,reply);
        },receiverUserID,reply);

    }else if(header.startsWith("ADD_FRIEND_SUCCESS")){
        QString sendUserID,receiveUserID,receiveUsername;
        messageStream >> sendUserID >> receiveUserID >>receiveUsername;

        sqlManipulation::instantiation()->add_friend(sendUserID,receiveUserID);
        QString header = "ADD_FRIEND_SUCCESS";
        replyStream<<header<< sendUserID<<receiveUserID<<receiveUsername;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](QString sendUserID,QByteArray reply){
            emit signalSendMessage(sendUserID,reply);
        },sendUserID,reply);

    }else if(header.startsWith("ADD_FRIEND_FAIL")){
        QString sendUserID,receiveUserID;
        messageStream >> sendUserID >> receiveUserID;

        QString header = "ADD_FRIEND_FAIL";
        replyStream<<header<< sendUserID<<receiveUserID;

        QtConcurrent::run(QThreadPool::globalInstance(),[this](QString sendUserID,QByteArray reply){
            emit signalSendMessage(sendUserID,reply);
        },sendUserID,reply);


    }else if(header.startsWith("CREATE_GROUP")){
        QString userID,groupname,groupID;
        messageStream>> userID>>groupname;

        groupID = sqlManipulation::instantiation()->create_group(userID,groupname);

        QString header = "CREATE_GROUP_SUCCESS";
        replyStream << header << groupID;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit signalSendMessage(descriptor,reply);
        },descriptor,reply);


    }else if(header.startsWith("PRIVATE_CHAT")){  //私聊

        //报文：发送者的userID、接收者的userID、时间、聊天内容
        QString sendUserID,receiveUserID,time,content;
        messageStream>>sendUserID>>receiveUserID>>time>>content;
        qDebug() <<time<< "  Client(QtId=" << sendUserID << ") send to "<< "Client(QtId=" << receiveUserID << "): " << content;
        // 将报文原样转发给接收者
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiveUserID, QByteArray message){
            emit signalSendMessage(receiveUserID, message);
        }, receiveUserID, message);

    }else if(header.startsWith("PUBLIC_CHAT")){  //群聊

        //报文：发送者的userID、群聊ID、时间、聊天内容
        QString userID,groupID,time,content;
        messageStream>>userID>>groupID>>time>>content;
        qDebug() <<time<< "  Client(QtId=" << userID << ") send to group "<< groupID << ": " << content;
        QList<QString> group_members=sqlManipulation::instantiation()->get_groupMember(groupID);
        // 将报文转发给群聊成员
        for(auto user_ID:group_members){
            QtConcurrent::run(QThreadPool::globalInstance(), [this](QString user_ID, QByteArray message){
                emit signalSendMessage(user_ID, message);
            }, user_ID, message);
        }

    }else if(header.startsWith("ADD_GROUP")){  //申请加群

        //报文：请求者的userID、请求者nickname、群聊ID
        //请求者向群聊创建者发送加群请求，执行结果会发给请求者
        QString userID,nickname,groupID;
        messageStream>>userID>>nickname>>groupID;
        qDebug() <<"Client(QtId=" << userID << ") apply for joining group "<< groupID;
        QString creatorID=sqlManipulation::instantiation()->get_creatorID(groupID);
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString creatorID, QByteArray message){
            emit signalSendMessage(creatorID, message);
        }, creatorID, message);

    }else if(header.startsWith("ADD_GROUP_SUCCESS")){  //群聊创建者同意加群请求

        //报文：请求者的userID、群聊ID、群聊名称
        QString userID,groupID,groupName;
        messageStream>>userID>>groupID>>groupName;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString userID, QString groupID, QByteArray message){
            QByteArray feedback;
            QDataStream stream(&feedback,QIODevice::WriteOnly);
            bool result=sqlManipulation::instantiation()->add_group(userID,groupID);
            if(result){  //成功插入Groups表
                emit signalSendMessage(userID, message);
                qDebug() <<"Client(QtId=" << userID << ") successfully join group "<< groupID;
            }
            else{  //插入Groups表失败
                QString fail="ADD_GROUP_FAIL";
                stream<<fail<<userID<<groupID;
                emit signalSendMessage(userID, feedback);
                qDebug() <<"Client(QtId=" << userID << ") fail to join group "<< groupID;
            }
        }, userID, groupID, message);

    }else if(header.startsWith("ADD_GROUP_FAIL")){  //群聊创建者拒绝加群请求

        //报文：请求者的userID、群聊ID
        QString userID,groupID;
        messageStream>>userID>>groupID;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString userID, QString groupID, QByteArray message){
            emit signalSendMessage(userID, message);
            qDebug() <<"Client(QtId=" << userID << ") fail to join group "<< groupID;
        }, userID, groupID, message);

    }else if(header.startsWith("INVITE_GROUP")){  //群聊创建者邀请加群

        //报文：被邀请者userID、邀请者nickname、群聊ID
        //只有群聊创建者能邀请别人加群，执行结果会发给创建者
        QString userID,inviter_nickname,groupID;
        messageStream>>userID>>inviter_nickname>>groupID;
        qDebug() <<"User (" << inviter_nickname << ") invite " << "Client(QtId=" << userID << ") to join group "<< groupID;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString userID, QByteArray message){
            emit signalSendMessage(userID, message);
        }, userID, message);

    }else if(header.startsWith("INVITE_GROUP_SUCCESS")){  //被邀请者同意加群

        //报文：被邀请者的userID、群聊ID、群聊名称
        QString userID,groupID,groupName;
        messageStream>>userID>>groupID>>groupName;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString userID, QString groupID, QByteArray message){
            QByteArray feedback;
            QDataStream stream(&feedback,QIODevice::WriteOnly);
            QString creatorID=sqlManipulation::instantiation()->get_creatorID(groupID);
            bool result=sqlManipulation::instantiation()->add_group(userID,groupID);
            if(result){  //成功插入Groups表
                emit signalSendMessage(creatorID,message);
                qDebug() <<"Client(QtId=" << userID << ") successfully join group "<< groupID;
            }
            else{  //插入Groups表失败
                QString fail="INVITE_GROUP_FAIL";
                stream<<fail<<userID<<groupID;
                emit signalSendMessage(creatorID,feedback);
                qDebug() <<"Client(QtId=" << userID << ") fail to join group "<< groupID;
            }
        }, userID, groupID, message);


    }else if(header.startsWith("INVITE_GROUP_FAIL")){  //被邀请者拒绝加群

        //报文：被邀请者的userID、群聊ID
        QString userID,groupID;
        messageStream>>userID>>groupID;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString userID, QString groupID, QByteArray message){
            QString creatorID=sqlManipulation::instantiation()->get_creatorID(groupID);
            emit signalSendMessage(creatorID,message);
            qDebug() <<"Client(QtId=" << userID << ") fail to join group "<< groupID;
        }, userID, groupID, message);

    }else if(header.startsWith("SEND_FILE")){  //发送文件

        //报文:发送者ID、接收者ID、文件名、文件大小、文件内容
        QString senderID,receiverID,filename;
        long long file_size;
        QByteArray content;
        messageStream>>senderID>>receiverID>>filename>>file_size>>content;
        qDebug() <<"Client(QtId=" << senderID << ") send "<<filename<<" to "<<"Client(QtId=" << receiverID << ")";
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiverID, QByteArray message){
            emit signalSendMessage(receiverID,message);
        }, receiverID, message);

    }else if(header.startsWith("SEND_IMAGE")){  //发送图片

        //报文:发送者ID、接收者ID、图片名、图片内容
        QString senderID,receiverID,imagename;
        QByteArray content;
        messageStream>>senderID>>receiverID>>imagename>>content;
        qDebug() <<"Client(QtId=" << senderID << ") send "<<imagename<<" to "<<"Client(QtId=" << receiverID << ")";
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiverID, QByteArray message){
            emit signalSendMessage(receiverID,message);
        }, receiverID, message);

    }
}

