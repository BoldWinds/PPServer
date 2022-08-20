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

    QByteArray read();

    void write(QByteArray message);

    void record_userID(QString userID);

    void record_descriptor(qintptr descriptor);

    void close();

    QAbstractSocket::SocketState state();




};

#endif // SERVERSOCKETTHREAD_H
