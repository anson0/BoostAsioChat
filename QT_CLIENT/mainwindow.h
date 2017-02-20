#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "clientclass.h"
#include <boost/asio.hpp>
#include <string>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();
    void readMessage(int str);

private:
    Ui::MainWindow *ui;
    boost::asio::io_service io_service;
    std::shared_ptr< chat_client> m_ptrClient;
};

#endif // MAINWINDOW_H
