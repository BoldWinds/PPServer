#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <serversingleton.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    //开启服务器
    void open_server();
    //关闭服务器
    void close_server();
    //在线用户新增
    void online_increase(QString userID);
    //在线用户减少
    void online_decrease(QString userID);


private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
