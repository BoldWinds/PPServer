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
#include <QDataStream>
#include <QMutex>

class sqlManipulation : public QObject
{
    Q_OBJECT
public:
    explicit sqlManipulation(QObject *parent = 0);
    //实例化sqlManipulation类的静态方法mail
    static sqlManipulation* instantiation();
    //使用nickname和password和mail注册账号
    QString register_account(QString nickname,QString password,QString mail);
    //使用userID和password登录账号
    bool login_account(QString userID,QString password);
    //获取用户好友和群聊列表
    QByteArray get_chatlist(QString userID);
    //获取好友列表
    QList<QString> get_friendlist(QString userID);
    //获取群聊列表
    QList<QString> get_grouplist(QString userID);
    //给userID1添加userID2的好友
    bool add_friend(QString userID1,QString userID2);
    //创建以userID为创建者的群聊
    QString create_group(QString userID,QString groupName);
    //按照groupID获取同一群组所有成员
    QList<QString> get_groupMember(QString groupID);
    //将userID加入到groupID群组中
    bool add_group(QString userID,QString groupID);
    //查询密码是否正确
    bool check_password(QString userID,QString password);
    //查询密保邮箱是否正确
    bool check_mail(QString userID,QString mail);
    //查询两人是否为好友
    bool check_friend(QString userID1,QString userID2);
    //修改用户密码
    bool change_password(QString userID,QString newPassword);
    //修改用户邮箱
    bool change_mail(QString userID,QString newMail);
    //修改用户昵称
    bool change_nickname(QString userID,QString newNickname);
    //修改用户头像
    bool change_profile(QString userID,QString newProfile);
    //通过userID获取对应nickname
    QString get_nickname(QString userID);
    //通过userID获取对应mail
    QString get_mail(QString userID);
    //通过userID获取对应profile
    QString get_profile(QString userID);
    //通过userID获取对应password
    QString get_password(QString userID);
    //通过groupID获取对应groupName
    QString get_groupName(QString groupID);
    //通过groupID获取对应creatorID
    QString get_creatorID(QString groupID);
    //输出表中内容
    void output_table(QString table,int column);
    //删除表
    void delete_table(QString table);

private:
    static sqlManipulation* new_instance;
    static int account_num;
    static int group_num;
    QSqlDatabase myDatabase;
};
//实例化sqlManipulation类的静态方法
inline sqlManipulation* sqlManipulation::instantiation(){
    QMutex mutex;
    if(new_instance==nullptr){
        mutex.lock();
        new_instance=new sqlManipulation;
        mutex.unlock();
    }
    return new_instance;
}

#endif // SQLMANIPULATION_H
