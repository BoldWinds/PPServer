#ifndef SERVERSOCKETTHREAD_H
#define SERVERSOCKETTHREAD_H

#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include "serversocket.h"

class ServerSocketThread : public QThread
{
    Q_OBJECT

public:
    explicit ServerSocketThread(QObject *parent = nullptr, qintptr descriptor = -1);
    ServerSocketThread(qintptr descriptor);

    //读socket中的内容
    QByteArray read();

    void write(QByteArray message);

    void record_userID(QString userID);

    void record_descriptor(qintptr descriptor);

    void close();

    QAbstractSocket::SocketState state();

protected:
    // run 重写继承自QThread类的run，在套接字保持连接时死循环
    virtual void run();

private:
    ServerSocket* serverSocket;      // 封装的自定义socket对象的指针
    QString userID = QString("");     // 保存的qtid
    qintptr descriptor = -1;          // 保存的socket描述符
    bool checkpoint = true;           // 标记循环终止

private slots:

    //slot_disconnected_qtid 接受socket断开的信号并转发    userID 断开连接的socket连接用户的userID
    void slot_disconnected_userID(QString userID);

    //slot_disconnected_des 接受socket断开的信号并转发  des 断开连接的socket的描述符
    void slot_disconnected_descriptor(qintptr des);

    //slot_readyread 接受socket收到信息的信号并转发des 收到信号的socket的描述符 message 收到的信息
    void slot_readyRead(qintptr descriptor, QByteArray message);

signals:

    //sig_disconnected_qtid 发送socket断开连接的信号 @param qtid 断开连接的socket连接用户的QtId
    void signal_disconnected_userID(QString userID);

    // sig_disconnected_des 发送socket断开连接的信号 des 断开连接的socket的描述符
    void signal_disconnected_descriptor(qintptr descriptor);

    // sig_readyRead 发送收到信息的信号，以开始读信息 des 收到信号的socket的描述符 message 收到的信息
    void signal_readyRead(qintptr descriptor, QByteArray message);

};

#endif // SERVERSOCKETTHREAD_H
