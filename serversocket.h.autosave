#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H
#include <QTcpSocket>

class ServerSocket : public QTcpSocket{

    Q_OBJECT

public:
    //构造函数
    explicit ServerSocket(QObject *parent = nullptr);

    //保存标识符
    void record_descriptor(qintptr descriptor);

    //保存userID
    void record_userID(QString userID);

private:
    //记录的descriptor
    qintptr descriptor = -1;
    //该socket所连接用户的ID
    QString userID = QString("");
signals:

    //断开连接的信号，准备处理断开连接用户的descriptor
    void signal_disconnected_descriptor(qintptr descriptor);

    //断开连接的信号，准备处理断开连接用户的userID
    void signal_disconnected_userID(QString userID);

    //收到信息的信号
    void signal_readyRead(qintptr descriptor,QByteArray message);

};

#endif // SERVERSOCKET_H
