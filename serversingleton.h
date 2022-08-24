#ifndef SERVERSINGLETON_H
#define SERVERSINGLETON_H

#include <QTcpServer>
#include <QHostAddress>
#include <QHostInfo>
#include <QTimerEvent>
#include <QMutex>
#include <QBuffer>
#include <QDir>
#include "serversocket.h"
#include "sqlmanipulation.h"
#include "serversocketthread.h"

class ServerSingleton : public QTcpServer
{

    Q_OBJECT

public:
    explicit ServerSingleton(QObject *parent = nullptr);
    //获取服务单例对象的指针
    static ServerSingleton* getInstance();

    //在本地以默认端口开启服务
    void openServer();

    //以提供的IP和端口开启服务
    void openServer(QString ip, QString port);

    //关闭服务
    void closeServer();

    //根据给定的descriptor，关闭对应连接的套接字
    void closeSocket(qintptr descriptor);

    // 根据给定的userID，关闭对应连接的套接字
    void closeSocket(QString userID);

    //将QByteArray类型图片转为png格式存在服务器，返回图片路径
    QString bytes_img(QByteArray img,QString userID);

    //将png类型图片转为QByteArray
    QByteArray img_bytes(QString path);

protected:
     // 有新的连接进入服务器时调用，自动为其分配描述符
    void incomingConnection(qintptr descriptor);

private:
    //Server单例的指针，防止重复创建
    static ServerSingleton* instance;
    //获取ID对应的nickname
    QString getNickname(QString userID);

    //Server的ip地址
    QString serverIP = "127.0.0.1";
    //从描述符到对应封装socket的QThread的hash
    QHash<qintptr, ServerSocketThread*> socketHash;
    //从userID到对应描述符的hash
    QHash<QString, qintptr> descriptorHash;
    //从userID到昵称的hash
    QHash<QString, QString> nicknameHash;
    //记录所有在线的用户
    QSet<QString> onlineSet;
    //从接收者userID到他的离线消息的hash
    QHash<QString,QList<QByteArray>> offlinemessageHash;
    //锁
    QMutex mutex;
signals:
    //特定userID发送信息信号
    void signalSendMessage(QString userID, const QByteArray message);

    //特定标识符发送消息信号
    void signalSendMessage(qintptr descriptor, QByteArray message);

    //用户上线信号
    void signalOnline(QString userID);

    //用户下线信号
    void signalOffline(QString userID);
private slots:

    //接收发送消息信号    向指定userID发送消息
    void slotSendMessage(QString userID, const QByteArray message);

    //接收发送消息信号    向指定标识符发送信息
    void slotSendMessage(qintptr descriptor, const QByteArray message);

    //处理用户上线的离线消息
    void slotUserOnline(QString userID);

    //接收ReadyRead信号    进行具体业务逻辑处理
    void slotReadMessage(qintptr descriptor, QByteArray message);

};
#endif // SERVERSINGLETON_H
