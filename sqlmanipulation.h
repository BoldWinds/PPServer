#ifndef SQLMANIPULATION_H
#define SQLMANIPULATION_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDebug>
#include <QCoreApplication>
#include <QSqlRecord>
#include <QSqlDriver>
#include <QSqlError>
#include <QMutex>

class sqlManipulation : public QObject
{
    Q_OBJECT
public:
    explicit sqlManipulation(QObject *parent = 0);
    //实例化sqlManipulation类的静态方法
    static sqlManipulation* instantiation();
    //使用nickname和password注册账号
    QString register_account(QString nickname,QString password);
    //使用userID和password登录账号
    bool login_account(QString userID,QString password);
    //获取用户好友和群聊列表
    QByteArray get_chatlist(QString userID);
    //给userID1添加userID2的好友
    bool add_friend(QString userID1,QString userID2);
    //创建以userID为创建者的群聊
    QString create_group(QString userID,QString groupName);
    //按照groupID获取同一群组所有成员
    QList<QString> get_groupMember(QString groupID);
    //将userID加入到groupID群组中，返回群组名
    QString add_group(QString useID,QString groupID);
    //通过userI获取对应nickname
    QString get_nickname(QString userID);


private:
    static sqlManipulation* new_instance;
};

inline sqlManipulation::instantiation(){
    QMutex mutex;
    if(new_instance==nullptr){
        mutex.lock();
        new_instance=new sqlManipulation;
        mutex.unlock();
    }
    return new_instance;
}

#endif // SQLMANIPULATION_H
