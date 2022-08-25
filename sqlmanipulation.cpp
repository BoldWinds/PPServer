#include "sqlmanipulation.h"
int sqlManipulation::account_num=0;
int sqlManipulation::group_num=0;
sqlManipulation* sqlManipulation::new_instance=nullptr;
sqlManipulation::sqlManipulation(QObject *parent) : QObject(parent)
{
    //建立数据库
    myDatabase=QSqlDatabase::addDatabase("QSQLITE","serverConnection");
    QString databaseName=QCoreApplication::applicationDirPath()+"/ChatDatabase.db";
    myDatabase.setDatabaseName(databaseName);
    bool ok=myDatabase.open();
    //当程序断开再次打开时，本地数据库还在，这时可以通过判断表是否存在
    //来更新account_num和group_num，便于继续分配id
    if(ok){  //成功打开数据库
        qDebug()<<"Successfully open database";
        QSqlQuery* query=new QSqlQuery(myDatabase);
        if(myDatabase.tables().contains("User")==false){
            //不存在User表，则进行创建
            account_num=0;
            QString create_user_table="CREATE TABLE User(userID char(8) PRIMARY KEY,"
                                      "nickname varchar(20),password varchar(20),mail varchar(30),profile varchar(40),groups varchar(60000))";
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
        if(myDatabase.tables().contains("Groups")==false){
            //不存在Groups表，则进行创建
            group_num=0;
            QString create_group_table="CREATE TABLE Groups(groupID char(8) PRIMARY KEY,groupName varchar(20),"
                                      "creatorID char(8),usersID varchar(60000))";
            if(query->prepare(create_group_table) && query->exec()){
                qDebug()<<"Successfully create TABLE Groups";
            }
            else qDebug()<<"Fail to create TABLE Groups";
        }
        else{  //Groups表已建立，更新group_num
            QString group_count=QString("SELECT COUNT(*) FROM Groups");
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
QString sqlManipulation::register_account(QString nickname,QString password,QString mail){
    account_num++;  //更新account_num
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString userID=QString("%1").arg(account_num,8,10,QLatin1Char('0'));  //8位ID
    QString insert_User=QString("INSERT INTO User (userID,nickname,password,mail) VALUES('%1','%2','%3','%4')").arg(userID).arg(nickname).arg(password).arg(mail);
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
    QString check_user=QString("SELECT * FROM User WHERE userID='%1' AND "
                               "password='%2'").arg(userID).arg(password);
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

//获取好友列表
QList<QString> sqlManipulation::get_friendlist(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QList<QString> friends_id;
    QString get_friend=QString("SELECT friend_userID FROM Friend WHERE userID='%1'").arg(userID);
    if(query->prepare(get_friend) && query->exec()){
        while(query->next()){
            QString friend_id=query->value(0).toString();
            friends_id.append(friend_id);
        }
        qDebug()<<"Successfully get friends list";
    }
    else qDebug()<<"Fail to get friends list";
    delete query;
    return friends_id;
}

//获取群聊列表
QList<QString> sqlManipulation::get_grouplist(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QList<QString> groups_id;
    QString get_group=QString("SELECT groups FROM User WHERE userID='%1'").arg(userID);
    if(query->prepare(get_group) && query->exec()){
        query->next();
        QString group_id=query->value(0).toString();
        //每8位分隔出一个groupID，并加入groups_id
        for(int i=0;i<group_id.length()/8;i++){
            QString str=group_id.mid(i*8,8);
            groups_id.append(str);
        }
        qDebug()<<"Successfully get groups list";
    }
    else qDebug()<<"Fail to get groups list";
    delete query;
    return groups_id;
}

//给userID1添加userID2的好友
bool sqlManipulation::add_friend(QString userID1,QString userID2){
    bool result=false;
    QSqlQuery* query=new QSqlQuery(myDatabase);
    //判断userID2是否存在
    QString search_id=QString("SELECT * FROM User WHERE userID='%1'").arg(userID2);
    if(query->prepare(search_id) && query->exec()){
        if(!query->next()){
            qDebug()<<"No such person";
            return result;
        }
    }
    //向Friend表中插入记录
    QString insert_friend = QString("INSERT INTO Friend VALUES('%1','%2')").arg(userID1).arg(userID2);
    if(query->prepare(insert_friend) && query->exec()){
        qDebug()<<"Successfully insert into Friend";
        result=true;
    }
    else qDebug()<<"Fail to insert into Friend";
    delete query;
    return result;
}

//创建以userID为创建者的群聊
QString sqlManipulation::create_group(QString userID,QString groupName){
    group_num++;  //更新group_num
    QSqlQuery* query=new QSqlQuery(myDatabase);
    //向Group表中插入记录
    QString groupID=QString("%1").arg(group_num,8,10,QLatin1Char('0'));
    QString usersID=userID;
    QString insert_Table=QString("INSERT INTO Groups VALUES('%1','%2','%3','%4')").arg(groupID).arg(groupName).arg(userID).arg(usersID);
    bool result1=query->prepare(insert_Table) && query->exec();
    //向User表中更新创建者groups属性记录
    QString groups=groupID;
    QString update_User=QString("UPDATE User SET groups='%1' WHERE userID='%2'").arg(groups).arg(userID);
    bool result2=query->prepare(update_User) && query->exec();
    if(result1&&result2){
        qDebug()<<"Successfully create group";
    }
    else qDebug()<<"Fail to create group";
    delete query;
    return groupID;
}

//按照groupID获取同一群组所有成员
QList<QString> sqlManipulation::get_groupMember(QString groupID){
    QList<QString> users_id;
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString get_users=QString("SELECT usersID FROM Groups WHERE groupID='%1'").arg(groupID);
    if(query->prepare(get_users) && query->exec()){
        query->next();
        QString user_id=query->value(0).toString();
        //每8位分隔出一个userID，并加入users_id
        for(int i=0;i<user_id.length()/8;i++){
            QString str=user_id.mid(i*8,8);
            users_id.append(str);
        }
        qDebug()<<"Successfully get users list";
    }
    else qDebug()<<"Fail to get users list";
    delete query;
    return users_id;
}

//将userID加入到groupID群组中
bool sqlManipulation::add_group(QString userID,QString groupID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    //判断该群组是否存在
    QString search_group=QString("SELECT * FROM Groups WHERE groupID='%1'").arg(groupID);
    if(query->prepare(search_group) && query->exec()){
        if(!query->next()){
            qDebug()<<"No such group";
            return false;
        }
    }

    //获取用户的groups属性
    QString get_groups=QString("SELECT groups FROM User WHERE userID='%1'").arg(userID);
    query->prepare(get_groups);
    query->exec();
    query->next();
    QString groups=query->value(0).toString();
    groups=groups+groupID;

    //向User表中更新groups属性记录
    QString update_User=QString("UPDATE User SET groups='%1' WHERE userID='%2'").arg(groups).arg(userID);
    bool result1=query->prepare(update_User) && query->exec();
    if(result1){
        qDebug()<<"Successfully update User";
    }
    else qDebug()<<"Fail to update User";

    //获取加入群组的usersID属性
    QString get_users=QString("SELECT usersID FROM Groups WHERE groupID='%1'").arg(groupID);
    query->prepare(get_users);
    query->exec();
    query->next();
    QString usersID=query->value(0).toString();
    usersID=usersID+userID;

    //向Groups表中更新usersID属性记录
    QString update_Group=QString("UPDATE Groups SET usersID='%1' WHERE groupID='%2'").arg(usersID).arg(groupID);
    bool result2=query->prepare(update_Group) && query->exec();
    if(result2){
        qDebug()<<"Successfully update Groups";
    }
    else qDebug()<<"Fail to update Groups";

    bool result=result1&&result2;
    delete query;
    return result;
}

//查询密码是否正确
bool sqlManipulation::check_password(QString userID,QString password){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString check_password=QString("SELECT * FROM User WHERE userID='%1' AND password='%2'").arg(userID).arg(password);
    bool result=false;
    if(query->prepare(check_password) && query->exec()){
        if(query->next()){
            result=true;
            qDebug()<<"User("<<userID<< ")'s password is correct";
        }
    }
    if(result==false){
        qDebug()<<"Fail to find user("<<userID<< ")'s password is wrong";
    }
    delete query;
    return result;
}

//查询密保邮箱是否正确
bool sqlManipulation::check_mail(QString userID,QString mail){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString check_mail=QString("SELECT * FROM User WHERE userID='%1' AND mail='%2'").arg(userID).arg(mail);
    bool result=false;
    if(query->prepare(check_mail) && query->exec()){
        if(query->next()){
            result=true;
            qDebug()<<"User("<<userID<< ")'s mail is correct";
        }
    }
    if(result==false){
        qDebug()<<"Fail to find user("<<userID<< ")'s mail is wrong";
    }
    delete query;
    return result;
}

//查询两人是否为好友
bool sqlManipulation::check_friend(QString userID1,QString userID2){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString check_friend=QString("SELECT * FROM Friend WHERE userID='%1' AND friend_userID='%2'").arg(userID1).arg(userID2);
    bool result=false;
    if(query->prepare(check_friend) && query->exec()){
        if(query->next()){
            result=true;
            qDebug()<<"User("<<userID1<< ") and User("<<userID2<<") are already friends";
        }
    }
    if(result==false){
        qDebug()<<"User("<<userID1<< ") and User("<<userID2<<") are not friends";
    }
    delete query;
    return result;
}

//查询群组是否存在
bool sqlManipulation::check_group(QString groupID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString check_group=QString("SELECT * FROM Groups WHERE groupID='%1'").arg(groupID);
    bool result=false;
    if(query->prepare(check_group) && query->exec()){
        if(query->next()){
            result=true;
            qDebug()<<"group exists";
        }
    }
    if(result==false){
        qDebug()<<"group not exists";
    }
    delete query;
    return result;
}

//修改用户密码
bool sqlManipulation::change_password(QString userID,QString newPassword){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString update_password=QString("UPDATE User SET password='%1' WHERE userID='%2'").arg(newPassword).arg(userID);
    bool result=query->prepare(update_password) && query->exec();
    if(result){
        qDebug()<<"Successfully retrieve";
    }
    else qDebug()<<"Fail to retrieve";
    delete query;
    return result;
}

//修改用户邮箱
bool sqlManipulation::change_mail(QString userID,QString newMail){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString update_mail=QString("UPDATE User SET mail='%1' WHERE userID='%2'").arg(newMail).arg(userID);
    bool result=query->prepare(update_mail) && query->exec();
    if(result){
        qDebug()<<"Successfully change mail";
    }
    else qDebug()<<"Fail to change mail";
    delete query;
    return result;
}

//修改用户昵称
bool sqlManipulation::change_nickname(QString userID,QString newNickname){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString update_nickname=QString("UPDATE User SET nickname='%1' WHERE userID='%2'").arg(newNickname).arg(userID);
    bool result=query->prepare(update_nickname) && query->exec();
    if(result){
        qDebug()<<"Successfully change nickname";
    }
    else qDebug()<<"Fail to change nickname";
    delete query;
    return result;
}

//修改用户头像
bool sqlManipulation::change_profile(QString userID,QString newProfile){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString update_profile=QString("UPDATE User SET profile='%1' WHERE userID='%2'").arg(newProfile).arg(userID);
    bool result=query->prepare(update_profile) && query->exec();
    if(result){
        qDebug()<<"Successfully change profile";
    }
    else qDebug()<<"Fail to change profile";
    delete query;
    return result;
}

//通过userID获取对应nickname
QString sqlManipulation::get_nickname(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString nickname;
    QString userID_nickname=QString("SELECT nickname FROM User WHERE userID='%1'").arg(userID);
    if(query->prepare(userID_nickname) && query->exec()){
        query->next();
        nickname=query->value(0).toString();
        qDebug()<<"Successfully get nickname";
    }
    else qDebug()<<"Fail to get nickname";
    delete query;
    return nickname;
}

//通过userID获取对应mail
QString sqlManipulation::get_mail(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString mail;
    QString userID_mail=QString("SELECT mail FROM User WHERE userID='%1'").arg(userID);
    if(query->prepare(userID_mail) && query->exec()){
        query->next();
        mail=query->value(0).toString();
        qDebug()<<"Successfully get mail";
    }
    else qDebug()<<"Fail to get mail";
    delete query;
    return mail;
}

//通过userID获取对应profile
QString sqlManipulation::get_profile(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString profile;
    QString userID_profile=QString("SELECT profile FROM User WHERE userID='%1'").arg(userID);
    if(query->prepare(userID_profile) && query->exec()){
        query->next();
        profile=query->value(0).toString();
        qDebug()<<"Successfully get profile";
    }
    else qDebug()<<"Fail to get profile";
    delete query;
    return profile;
}

//通过userID获取对应password
QString sqlManipulation::get_password(QString userID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString password;
    QString userID_password=QString("SELECT password FROM User WHERE userID='%1'").arg(userID);
    if(query->prepare(userID_password) && query->exec()){
        query->next();
        password=query->value(0).toString();
        qDebug()<<"Successfully get password";
    }
    else qDebug()<<"Fail to get password";
    delete query;
    return password;
}

//通过groupID获取对应groupName
QString sqlManipulation::get_groupName(QString groupID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString groupName;
    QString groupID_groupName=QString("SELECT groupName FROM Groups WHERE groupID='%1'").arg(groupID);
    if(query->prepare(groupID_groupName) && query->exec()){
        query->next();
        groupName=query->value(0).toString();
        qDebug()<<"Successfully get groupName";
    }
    else qDebug()<<"Fail to get groupName";
    delete query;
    return groupName;
}

//通过groupID获取对应creatorID
QString sqlManipulation::get_creatorID(QString groupID){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString creatorID;
    QString groupID_creatorID=QString("SELECT creatorID FROM Groups WHERE groupID='%1'").arg(groupID);
    if(query->prepare(groupID_creatorID) && query->exec()){
        query->next();
        creatorID=query->value(0).toString();
        qDebug()<<"Successfully get groupName";
    }
    else qDebug()<<"Fail to get groupName";
    delete query;
    return creatorID;
}

//输出表中内容
void sqlManipulation::output_table(QString table,int column){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString str;
    QString select_all=QString("SELECT * FROM %1").arg(table);
    qDebug()<<table<<":";
    if(query->prepare(select_all) && query->exec()){
        while(query->next()){
            str=query->value(0).toString();
            for(int i=1;i<column;i++){
                if(query->isNull(i)){
                    str=str+"  "+"NULL";
                }
                else{
                    str=str+"  "+query->value(i).toString();
                }
            }
            qDebug()<<str;
        }
    }
    delete query;
}

//删除表
void sqlManipulation::delete_table(QString table){
    QSqlQuery* query=new QSqlQuery(myDatabase);
    QString delete_content=QString("DELETE FROM %1").arg(table);
    if(query->prepare(delete_content) && query->exec()){
        qDebug()<<"Successfully delete "<<table;
    }
    else qDebug()<<"Fail to delete "<<table;
    delete query;
}

