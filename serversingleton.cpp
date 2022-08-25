#include <QDataStream>
#include <future>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFile>
#include "serversingleton.h"

ServerSingleton* ServerSingleton::instance = nullptr;

ServerSingleton::ServerSingleton(QObject *parent) : QTcpServer(parent)
{

    //发送消息的connet函数
    connect(this, SIGNAL(signalSendMessage(QString, const QByteArray)),
            this, SLOT(slotSendMessage(QString, const QByteArray)));

    connect(this,SIGNAL(signalOnline(QString)),
                        this,SLOT(slotUserOnline(QString)));

    if(!QMetaType::isRegistered((QMetaType::type("qintptr")))){
        qRegisterMetaType<qintptr>("qintptr");
    }

    connect(this, SIGNAL(signalSendMessage(qintptr, const QByteArray)),
            this, SLOT(slotSendMessage(qintptr, const QByteArray)));
}


ServerSingleton* ServerSingleton::getInstance(){
    if(ServerSingleton::instance == nullptr){
        QMutex mutex;
        //使用锁确保只有一个单例
        mutex.lock();
        ServerSingleton::instance = new ServerSingleton();
        mutex.unlock();
    }
    return ServerSingleton::instance;
}


void ServerSingleton::openServer(QString ip,QString port){
    if(this->isListening()){
        qDebug()<<"server has already been listening";
    }else if(this->listen(QHostAddress::Any, port.toUInt())){
        qDebug() << "Server listening...";
        qDebug() << "Server ip is" << ip;
        qDebug() << "Server port is " + port;
    }else{
        qWarning() << "server can not open!";
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

        //停止循环
        serverSocketThread->close();
        //退出
        serverSocketThread->quit();
        //确保退出
        serverSocketThread->wait();
        //释放资源
        serverSocketThread->deleteLater();

        qDebug()<<"socket: "<<descriptor<<" closed";
        socketHash.remove(descriptor);
    }

}

void ServerSingleton::closeSocket(QString userID){
    if(descriptorHash.find(userID) == descriptorHash.end()){
        qDebug() << "Client(userID=" << userID << ") is offline, need not to close.";
    }else{
        qDebug() << "Closing socket of client(userID=" << userID << ")...";
        closeSocket(descriptorHash[userID]);
        emit signalOffline(userID);
        qDebug() << "Socket of client(userID=" << userID << ") closed.";
    }
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
    //qDebug() << "Sending message: " << message;
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
       qDebug() << "Sending message";

       ServerSocketThread* serverSocketThread = socketHash[descriptor];
       serverSocketThread->write(message);
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

    if(header=="REGISTER"){
        QImage profile;
        QString nickname,password,mail;
        messageStream >> nickname >> password >> mail >> profile;

        QString userID = sqlManipulation::instantiation()->register_account(nickname,password,mail);

        QString profile_path=bytes_img(profile,userID);
        sqlManipulation::instantiation()->change_profile(userID,profile_path);

        qDebug()<<"user: "<<nickname <<"'s userID is: "<<userID;

        QString header = "REGISTER_SUCCESS";
        replyStream<<header<<userID;

        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);

    }else if(header=="LOGIN"){
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

            //Hash操作
            onlineSet.insert(userID);
            descriptorHash[userID] = descriptor;

            emit signalOnline(userID);

            //给socket记录userID
            ServerSocketThread *serverSocketThread = socketHash[descriptor];
            serverSocketThread->recordUserID(userID);

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

        }else{
            QString header = "LOGIN_FAIL";
            replyStream<<header;
            emit signalSendMessage(descriptor,reply);
            qDebug()<<"user: "<<userID<<" login fail";
        }
    }else if(header=="RETRIEVE"){
        QString userID,mail,password;
        messageStream >> userID >> mail >> password;
        qDebug()<<"user: "<<userID << "try to retrieve";
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
    }else if(header=="GET_USER_INFO"){
        QString userID;
        messageStream >> userID;

        QString NickName,Mail;
        QImage profile;
        NickName=sqlManipulation::instantiation()->get_nickname(userID);
        Mail=sqlManipulation::instantiation()->get_mail(userID);
        QString profile_path=sqlManipulation::instantiation()->get_profile(userID);
        profile=img_bytes(profile_path);

        int friendsCount,groupsCount;

        sqlManipulation* sql = sqlManipulation::instantiation();

        QList<QString> friendIDs = sql->get_friendlist(userID);
        QList<QString> groupIDs = sql->get_grouplist(userID);

        friendsCount = friendIDs.size();
        groupsCount = groupIDs.count();

        QList<QString> friendnames = QList<QString>();
        QList<QString> groupnames = QList<QString>();
        QList<QImage> profiles = QList<QImage>();

        for(auto friendID : friendIDs){
            friendnames.append(sql->get_nickname(friendID));
            QString profile_path=sql->instantiation()->get_profile(friendID);
            profiles.append(img_bytes(profile_path));
        }
        for(auto groupID : groupnames){
            groupnames.append(sql->get_groupName(groupID));
        }

        QString header = "GET_USER_INFO_SUCCESS";
        replyStream << header << NickName << Mail << profile<<friendsCount<<friendnames<<friendIDs<<profiles
                    <<groupsCount<<groupnames<<groupIDs;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);

    }/*else if(header.startsWith("GET_FRIENDS")){
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
        QList<QImage> profiles = QList<QImage>();

        for(auto friendID : friendIDs){
            friendnames.append(sql->get_nickname(friendID));
            QString profile_path=sql->instantiation()->get_profile(friendID);
            profiles.append(img_bytes(profile_path));
        }
        for(auto groupID : groupnames){
            groupnames.append(sql->get_groupName(groupID));
        }

        QString header = "GET_FRIENDS_SUCCESS";
        replyStream<<header<<friendsCount<<friendnames<<friendIDs<<profiles
                  <<groupsCount<<groupnames<<groupIDs;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit signalSendMessage(descriptor,reply);
        },descriptor,reply);

    }*/else if(header=="UPDATE_USERINFO"){
        QImage Profile;  //头像非空
        QString NickName,Mail,NewPassword,OriginalPassword;
        messageStream>>NickName>>Mail>>NewPassword>>OriginalPassword>>Profile;
        QString userID=descriptorHash.key(descriptor);
        bool result=sqlManipulation::instantiation()->check_password(userID,OriginalPassword);
        if(result){
            if(!NickName.isEmpty()) sqlManipulation::instantiation()->change_nickname(userID,NickName);
            if(!Mail.isEmpty()) sqlManipulation::instantiation()->change_mail(userID,Mail);
            if(!NewPassword.isEmpty()) sqlManipulation::instantiation()->change_password(userID,NewPassword);
            bytes_img(Profile,userID);  //保存图片，无需修改数据库中路径
        }

        QString header = "UPDATE_USERINFO_SUCCESS";
        QString nickname=sqlManipulation::instantiation()->get_nickname(userID);
        QString mail=sqlManipulation::instantiation()->get_mail(userID);
        QString password=sqlManipulation::instantiation()->get_password(userID);
        QString profile_path=sqlManipulation::instantiation()->get_profile(userID);
        QImage profile=img_bytes(profile_path);
        replyStream<<header<<nickname<<mail<<password<<profile;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit ServerSingleton::signalSendMessage(descriptor,reply);
        },descriptor,reply);
    }else if(header=="ADD_FRIEND"){
        QString sendUserID,sendUsername,receiverUserID;
        messageStream >> sendUserID>>sendUsername>>receiverUserID;

        bool result=sqlManipulation::instantiation()->check_friend(sendUserID,receiverUserID);
        if(result){
            QString header="ADD_FRIEND_CONFLICT";
            replyStream<<header;
            QtConcurrent::run(QThreadPool::globalInstance(),[this](QString sendUserID,QByteArray reply){
                emit signalSendMessage(sendUserID,reply);
            },sendUserID,reply);
        }
        else{
            QString header = "ADD_FRIEND";
            QImage profile;
            QString profile_path=sqlManipulation::instantiation()->get_profile(sendUserID);
            profile=img_bytes(profile_path);
            replyStream<<header<< sendUserID<<sendUsername<<receiverUserID<<profile;
            QtConcurrent::run(QThreadPool::globalInstance(),[this](QString receiverUserID,QByteArray reply){
                emit signalSendMessage(receiverUserID,reply);
            },receiverUserID,reply);
        }


    }else if(header=="ADD_FRIEND_SUCCESS"){
        QImage receiverProfile;
        QString sendUserID,receiveUserID,receiveUsername;
        messageStream >> sendUserID >>receiveUsername>> receiveUserID>>receiverProfile;

        sqlManipulation::instantiation()->add_friend(sendUserID,receiveUserID);
        sqlManipulation::instantiation()->add_friend(receiveUserID,sendUserID);

        QString header = "ADD_FRIEND_SUCCESS";
        replyStream<<header<< sendUserID<<receiveUsername<<receiveUserID<<receiverProfile;

        QtConcurrent::run(QThreadPool::globalInstance(),[this](QString sendUserID,QByteArray reply){
            emit signalSendMessage(sendUserID,reply);
        },sendUserID,reply);

    }else if(header=="ADD_FRIEND_FAIL"){
        QString sendUserID,receiveUserID;
        messageStream >> sendUserID >> receiveUserID;

        QString header = "ADD_FRIEND_FAIL";
        replyStream<<header<< sendUserID<<receiveUserID;

        QtConcurrent::run(QThreadPool::globalInstance(),[this](QString sendUserID,QByteArray reply){
            emit signalSendMessage(sendUserID,reply);
        },sendUserID,reply);


    }else if(header=="CREATE_GROUP"){
        QString userID,groupname,groupID;
        messageStream>> userID>>groupname;

        groupID = sqlManipulation::instantiation()->create_group(userID,groupname);

        QString header = "CREATE_GROUP_SUCCESS";
        replyStream << header << groupID;
        QtConcurrent::run(QThreadPool::globalInstance(),[this](qintptr descriptor,QByteArray reply){
            emit signalSendMessage(descriptor,reply);
        },descriptor,reply);


    }else if(header=="PRIVATE_CHAT"){  //私聊

        //报文：发送者的userID、接收者的userID、时间、聊天内容
        QString sendUserID,receiveUserID,time,content;
        messageStream>>sendUserID>>receiveUserID>>time>>content;
        qDebug() <<time<< "  Client(QtId=" << sendUserID << ") send to "<< "Client(QtId=" << receiveUserID << "): " << content;
        // 将报文原样转发给接收者
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiveUserID, QByteArray message){
            emit signalSendMessage(receiveUserID, message);
        }, receiveUserID, message);

    }else if(header=="PUBLIC_CHAT"){  //群聊

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

    }else if(header=="ADD_GROUP"){  //申请加群

        //报文：请求者的userID、请求者nickname、群聊ID
        //请求者向群聊创建者发送加群请求，执行结果会发给请求者
        QString userID,nickname,groupID;
        messageStream>>userID>>nickname>>groupID;
        qDebug() <<"Client(QtId=" << userID << ") apply for joining group "<< groupID;

        QList<QString> groups = sqlManipulation::instantiation()->get_grouplist(userID);
        if(groups.contains(groupID)){
            //已经加群
            QString header = "ADD_GROUP_CONFLICT";
            replyStream << header;
            QtConcurrent::run(QThreadPool::globalInstance(), [this](qintptr des, QByteArray reply){
                emit signalSendMessage(des,reply);
            }, descriptor, reply);
        }else{
            QString creatorID=sqlManipulation::instantiation()->get_creatorID(groupID);
            QtConcurrent::run(QThreadPool::globalInstance(), [this](QString creatorID, QByteArray message){
                emit signalSendMessage(creatorID, message);
            }, creatorID, message);
        }

    }else if(header=="ADD_GROUP_SUCCESS"){  //群聊创建者同意加群请求

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

    }else if(header=="ADD_GROUP_FAIL"){  //群聊创建者拒绝加群请求

        //报文：请求者的userID、群聊ID
        QString userID,groupID;
        messageStream>>userID>>groupID;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString userID, QString groupID, QByteArray message){
            emit signalSendMessage(userID, message);
            qDebug() <<"Client(QtId=" << userID << ") fail to join group "<< groupID;
        }, userID, groupID, message);

    }else if(header=="INVITE_GROUP"){  //群聊创建者邀请加群

        //报文：邀请者ID、邀请者nickname、被邀请者userID、groupName、groupID
        //只有群聊创建者能邀请别人加群，执行结果会发给创建者
        QString inviterID,inviterName,receiverID,groupID,groupName;
        messageStream>>inviterID>>inviterName>>receiverID>>groupName>>groupID;
        qDebug() <<"User (" << inviterName << ") invite " << "Client(QtId=" << receiverID << ") to join group "<< groupName;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiverID, QByteArray message){
            emit signalSendMessage(receiverID, message);
        }, receiverID, message);

    }else if(header=="INVITE_GROUP_SUCCESS"){  //被邀请者同意加群

        //报文：邀请者的userID、被邀请者的userID、groupName、groupID
        QString inviterID,receiverID,groupName,groupID;
        messageStream>>inviterID>>receiverID>>groupName>>groupID;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString inviterID, QString receiverID, QString groupName,QString groupID,QByteArray message){
            QByteArray feedback;
            QDataStream stream(&feedback,QIODevice::WriteOnly);
            bool result=sqlManipulation::instantiation()->add_group(receiverID,groupID);
            if(result){  //成功插入Groups表
                emit signalSendMessage(inviterID,message);
                qDebug() <<"Client(QtId=" << receiverID << ") successfully join group "<< groupName;
            }
            else{  //插入Groups表失败
                QString fail="INVITE_GROUP_FAIL";
                stream<<fail<<inviterID<<receiverID<<groupName;
                emit signalSendMessage(inviterID,feedback);
                qDebug() <<"Client(QtId=" << receiverID << ") fail to join group "<< groupName;
            }
        }, inviterID, receiverID, groupName, groupID, message);


    }else if(header=="INVITE_GROUP_FAIL"){  //被邀请者拒绝加群

        //报文：邀请者的userID、被邀请者的userID、groupName
        QString inviterID,receiverID,groupName;
        messageStream>>inviterID>>receiverID>>groupName;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString inviterID, QString receiverID, QString groupName, QByteArray message){
            emit signalSendMessage(inviterID,message);
            qDebug() <<"Client(QtId=" << receiverID << ") fail to join group "<< groupName;
        }, inviterID, receiverID, groupName, message);

    }else if(header=="SEND_FILE"){  //发送文件

        //报文:发送者ID、接收者ID、文件名、文件大小、文件内容
        QString senderID,receiverID,filename;
        long long file_size;
        QByteArray content;
        messageStream>>senderID>>receiverID>>filename>>file_size>>content;
        qDebug() <<"Client(QtId=" << senderID << ") send "<<filename<<" to "<<"Client(QtId=" << receiverID << ")";
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiverID, QByteArray message){
            emit signalSendMessage(receiverID,message);
        }, receiverID, message);

    }else if(header=="SEND_IMAGE"){  //发送图片

        //报文:发送者ID、接收者ID、图片名、图片内容
        QString senderID,receiverID,imagename;
        QByteArray content;
        messageStream>>senderID>>receiverID>>imagename>>content;
        qDebug() <<"Client(QtId=" << senderID << ") send "<<imagename<<" to "<<"Client(QtId=" << receiverID << ")";
        QtConcurrent::run(QThreadPool::globalInstance(), [this](QString receiverID, QByteArray message){
            emit signalSendMessage(receiverID,message);
        }, receiverID, message);

    }else{  //针对所有不合法请求
        QString header = "ILLEGAL_MESSAGE";
        replyStream << header;
        QtConcurrent::run(QThreadPool::globalInstance(), [this](qintptr descriptor, QByteArray reply){
            emit signalSendMessage(descriptor,reply);
        }, descriptor,reply);
    }
}

//将QImage类型图片转为png格式存在服务器，返回图片路径
QString ServerSingleton::bytes_img(QImage img,QString userID){
    QString path_default=QDir::currentPath();
    QString path=path_default+"/"+userID+".png";
    if(img.save(path)){
        qDebug()<<"bytes to img success";
    }
    else{
        qDebug()<<"bytes to img fail";
    }
    return path;
}

//将png类型图片转为QImage
QImage ServerSingleton::img_bytes(QString path){
    QImage img;
    if(img.load(path)){
        qDebug()<<"img to bytes success";
    }
    else{
        qDebug()<<"img to bytes fail";
    }
    return img;
}

