#include "sqlmanipulation.h"
int sqlManipulation::account_num=0;
int sqlManipulation::group_num=0;
sqlManipulation* sqlManipulation::new_instance=nullptr;
sqlManipulation::sqlManipulation(QObject *parent) : QObject(parent)
{
    myDatabase=QSqlDatabase::addDatabase("QMYSQL","serverConnection");
    myDatabase.setDatabaseName("chatDatabase");
    bool ok=myDatabase.open();
    if(ok){  //成功打开数据库
        QSqlQuery* query=new QSqlQuery(myDatabase);
        if(myDatabase.tables().contains("User")==false){
            //不存在User表，则进行创建
            account_num=0;
            QString create_user_table="CREATE TABLE User(userID char(8) PRIMARY KEY,"
                                      "nickname varchar(20),password varchar(20),groups varchar(60000))";
            if(query->prepare(create_user_table) && query->exec()){
                qDebug()<<"Successfully create TABLE User";
            }
            else qDebug()<<"Fail to create TABLE User";
        }
        else{  //User表已建立，更新account_num
            QString user_count=QString("SELECT COUNT(*) FROM User");
            query->prepare(user_count);
            query->exec();
            query->next();
            account_num=query->value(0).toUInt();
        }
        if(myDatabase.tables().contains("Friend")==false){
            //不存在Friend表，则进行创建
            QString create_friend_table="CREATE TABLE Friend(userID char(8),"
                                      "friend_userID char(8))";
            if(query->prepare(create_friend_table) && query->exec()){
                qDebug()<<"Successfully create TABLE Friend";
            }
            else qDebug()<<"Fail to create TABLE Friend";
        }
        if(myDatabase.tables().contains("Group")==false){
            //不存在Group表，则进行创建
            group_num=0;
            QString create_group_table="CREATE TABLE Group(groupID char(8) PRIMARY KEY,groupName varchar(20),"
                                      "creatorID char(8),usersID varchar(60000))";
            if(query->prepare(create_group_table) && query->exec()){
                qDebug()<<"Successfully create TABLE Group";
            }
            else qDebug()<<"Fail to create TABLE Group";
        }
        else{  //Group表已建立，更新group_num
            QString group_count=QString("SELECT COUNT(*) FROM Group");
            query->prepare(group_count);
            query->exec();
            query->next();
            group_num=query->value(0).toUInt();
        }
        delete query;
    }
    else qDebug()<<"Fail to open database";
}

//使用nickname和password注册账号,并返回userID
QString sqlManipulation::register_account(QString nickname,QString password){
    account_num++;  //更新account_num
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString userID=QString("%1").arg(account_num,8,10,QLatin1Char('0'));
    QString insert_User=QString("INSERT INTO User VALUES('%1','%2','%3')").arg(userID).arg(nickname).arg(password);
    if(query->prepare(insert_User) && query->exec()){
        qDebug()<<"Successfully register";
    }
    else qDebug()<<"Fail to register";
    delete query;
    return userID;
}

//使用userID和password登录账号
bool sqlManipulation::login_account(QString userID,QString password){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    bool result=false;
    QString check_user=QString("SELECT * FROM User WHERE userID=%1 AND "
                               "password=%2").arg(userID).arg(password);
    if(query->prepare(check_user) && query->exec()){
        if(query->next()){
            qDebug()<<"Successfully login";
            result=true;
        }
    }
    else qDebug()<<"Fail to login";
    delete query;
    return result;
}

//获取用户好友和群聊列表
QByteArray sqlManipulation::get_chatlist(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    int friends_num,groups_num;
    QList<QString> friends_id,friends_nickname,groups_id,groups_name;
    //获取好友列表
    QString get_friend=QString("SELECT friend_userID FROM Friend WHERE userID=%1").arg(userID);
    if(query->prepare(get_friend) && query->exec()){
        while(query->next()){
            QString friend_id=query->value(0).toString();
            friends_id.append(friend_id);
            friends_nickname.append(get_nickname(friend_id));
        }
        qDebug()<<"Successfully get friends list";
    }
    else qDebug()<<"Fail to get friends list";
    //获取群聊列表
    QString get_group=QString("SELECT groups FROM User WHERE userID=%1").arg(userID);
    if(query->prepare(get_group) && query->exec()){
        query->next();
        QString group_id=query->value(0).toString();
        for(int i=0;i<group_id.length()/8;i++){
            QString str=group_id.mid(i*8,8);
            groups_id.append(str);
            groups_name.append(get_groupName(str));
        }
        qDebug()<<"Successfully get groups list";
    }
    else qDebug()<<"Fail to get groups list";
    //获取QByteArray类型返回值
    friends_num=friends_id.length();
    groups_num=groups_id.length();
    QByteArray result;
    QDataStream stream(&result,QIODevice::WriteOnly);
    stream<<friends_num;
    stream<<friends_id;
    stream<<friends_nickname;
    stream<<groups_num;
    stream<<groups_id;
    stream<<groups_name;
    return result;
}

//给userID1添加userID2的好友
bool sqlManipulation::add_friend(QString userID1,QString userID2){
    bool result=false;
    QSqlQuery* query=new QSqlQuery(myDatabase);
    //判断该用户是否存在
    QString search_id=QString("SELECT * FROM User WHERE userID=%1").arg(userID2);
    if(query->prepare(search_id) && query->exec()){
        if(!query->next()){
            qDebug()<<"No such person";
            return result;
        }
    }
    //向Friend表中插入记录
    QString insert_friend = QString("INSERT INTO Friend VALUES(%1,%2)").arg(userID1).arg(userID2);
    if(query->prepare(insert_friend) && query->exec()){
        qDebug()<<"Successfully insert into Friend";
        result=true;
    }
    else qDebug()<<"Fail to insert into Friend";
    return result;
}

//创建以userID为创建者的群聊
QString sqlManipulation::create_group(QString userID,QString groupName){
    group_num++;  //更新group_num
    QSqlQuery* query=new QSqlQuery(myDatabase);
    //向Group表中插入记录
    QString groupID=QString("%1").arg(group_num,8,10,QLatin1Char('0'));
    QString usersID=userID;
    QString insert_Table=QString("INSERT INTO Table VALUES('%1','%2','%3','%4')").arg(groupID).arg(groupName).arg(userID).arg(usersID);
    bool result1=query->prepare(insert_Table) && query->exec();
    //向User表中更新创建者groups属性记录
    QString groups=groupID;
    QString update_User=QString("UPDATE User SET groups=%1 WHERE userID=%2").arg(groups).arg(userID);
    bool result2=query->prepare(update_User) && query->exec();
    if(result1&&result2){
        qDebug()<<"Successfully create group";
    }
    else qDebug()<<"Fail to create group";
    delete query;
    return userID;
}


