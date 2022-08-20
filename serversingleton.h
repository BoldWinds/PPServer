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
    ServerSingleton();
    /*
     * @brief get_instance 获取服务单例对象的指针

    static ServerSingleton* get_instance();
    *
     * @brief get_network_info 获取本地网络消息

    void get_network_info();

    //再本地以默认端口开启服务
    void openServer();

    *
     * @brief open_server 以gui上的ip和端口号开启服务
     * @param ip
     * @param port

    void open_server(QString ip, QString port);

     * @brief close_server 关闭当前服务

    void close_server();
    /**
     * @brief close_socket 根据给定的socketDescriptor，关闭对应连接的套接字
     * @param des 描述符

    void close_socket(qintptr des);
    /**
     * @brief close_socket 根据给定的QtId，关闭对应连接的套接字
     * @param qtid

    void close_socket(QtId qtid);

protected:
    /**
     * @brief incomingConnection override 有新的连接进入服务器时调用，自动为其分配描述符
     * @param des 描述符

    void incomingConnection(qintptr description);
    /**
     * @brief timerEvent 定时器，检测心跳信号
     * @param e timer

//    virtual void timerEvent(QTimerEvent* e);

private:

    explicit ServerSingleton(QObject *parent = nullptr);

    QString get_nickname(QString userID);

    //Server单例的指针，防止重复创建
    static ServerSingleton* instance;
    //Server的ip地址
    QString server_ip = "127.0.0.1";
//    QHash<qintptr, ServerSocketThread*> socket_hash;                    // 从描述符到对应封装socket的QThread的hash
//    QHash<QtId, qintptr> descriptor_hash;                               // 从QtId到对应描述符的hash
//    QHash<QtId, QString> nickname_hash;                                 // 从QtId到昵称的hash，加快查找速度
//    QHash<QtId, quint16> heart_hash;                                    // 记录当前用户心跳包状态的hash
//    QHash<QPair<QtId, QtId> , QList<QString>* > message_cache_hash;     // 从发送者到接收者QtId的QPair到对应离线消息的hash
    QSet<QString> online_set;                                              // 记录在线客户端的id
    QHostAddress hostaddr;                                              // host地址
    QHostInfo hostinfo;                                                 // host信息
//    int heart_timer;                                                    // 检测心跳包的timer，每10s进行检测
    QMutex mutex;                                                       // 锁
*/
};
#endif // SERVERSINGLETON_H
