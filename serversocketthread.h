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

    //发送
    void write(QByteArray message);

    //保存userID
    void recordUserID(QString userID);

    //保存标识符
    void recordDescriptor(qintptr descriptor);

    //关闭连接
    void close();

    //该线程socket的状态
    QAbstractSocket::SocketState state();

protected:
    // run 重写继承自QThread类的run，在套接字保持连接时死循环
    virtual void run();

private:
    //封装的自定义socket对象的指针
    ServerSocket* serverSocket;
    //保存的userID
    QString userID = QString("");
    //保存的socket标识符
    qintptr descriptor = -1;
    //用于标记循环终止
    bool checkpoint = true;

private slots:

    //接受socket断开的信号并转发给singleton
    void slotDisconnectedUserID(QString userID);

    //接受socket断开的信号并转发给singleton
    void slotDisconnectedDescriptor(qintptr des);

    //接受socket收到信息的信号并转发给singleton
    void slotReadyRead(qintptr descriptor, QByteArray message);

signals:

    //该信号要通知给singleton socket断开连接的信号
    void signalDisconnectedUserID(QString userID);

    //该信号要通知给singleton socket断开连接的信号
    void signalDisconnectedDescriptor(qintptr descriptor);

    //该信号要通知给singleton 读取信号
    void signalReadyRead(qintptr descriptor, QByteArray message);

};

#endif // SERVERSOCKETTHREAD_H
