#ifndef SERVERSINGLETON_H
#define SERVERSINGLETON_H

#include <QTcpServer>
#include <QHostAddress>
#include <QHostInfo>
#include <QTimerEvent>
#include <QMutex>
#include "serversocket.h"
#include "sqlmanipulation.h"
#include "serversocketthread.h"

class ServerSingleton : public QTcpServer
{
public:
    explicit ServerSingleton(QObject *parent = nullptr);
    //获取服务单例对象的指针
    static ServerSingleton* getInstance();

    //获取本地网络消息
    void getNetworkInfo();

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

protected:

     // 有新的连接进入服务器时调用，自动为其分配描述符
    void newConnection(qintptr descriptor);

    // timerEvent 定时器，检测心跳信号
//    virtual void timerEvent(QTimerEvent* e);

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
    // host地址
    QHostAddress hostaddr;
    // host信息
    QHostInfo hostinfo;
//    QHash<QtId, quint16> heart_hash;                                    // 记录当前用户心跳包状态的hash
//    QHash<QPair<QtId, QtId> , QList<QString>* > message_cache_hash;     // 从发送者到接收者QtId的QPair到对应离线消息的hash
//    int heart_timer;                                                    // 检测心跳包的timer，每10s进行检测
    QMutex mutex;                                                       // 锁
signals:
    //特定userID发送信息信号
    void signalSendMessage(QString userID, const QByteArray message);

    //特定标识符发送消息信号
    void sigSendMessage(qintptr descriptor, QByteArray message);

    //获取标识符信号
    void signalGetIPList(QHostInfo hostInfo);

    //用户上线信号
    void signalOnline(QString userID);

    //用户下线信号
    void sigOffline(QString userID);

    //更新gui信号
    void signalUpdateGui(QString log);
private slots:

    //接收发送消息信号    向指定userID发送消息
    void slotSendMessage(QString userID, const QByteArray message);

    //接收发送消息信号    向指定标识符发送信息
    void slotSendMessage(qintptr descriptor, const QByteArray message);

    // 接受获取ip的信号    获取本机的所有ip
    void slotGetAddress(QHostInfo hostInfo);
};
#endif // SERVERSINGLETON_H