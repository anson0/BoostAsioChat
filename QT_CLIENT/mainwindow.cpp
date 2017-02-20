#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <mutex>
#include "clientclass.h"
std::mutex mu;
std::string g_str="";
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->page);

    QPixmap pix(":/new/prefix1/download.jpg");
   // ui->label->setStyelSheet("border-image:url(:/2.png);");
    ui->label_3->setPixmap(pix);
    m_bUserSaid=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_2_clicked()
{
    tcp::resolver resolver(io_service);
    QString strIp= ui->textEdit->toPlainText();
    QString strUser=ui->textEdit_2->toPlainText();
    QByteArray ba = strIp.toLatin1();
    const char *c_strIp = ba.data();
    QByteArray baUser = strUser.toLatin1();
    const char *c_strUser = baUser.data();
    m_strUserName.assign(c_strUser,std::find(c_strUser,c_strUser+strlen(c_strUser),'\0'));
    auto endpoint_iterator = resolver.resolve({c_strIp,"1234"});//({ argv[1], argv[2] });
    m_ptrClient=std::make_shared<chat_client> (io_service, endpoint_iterator);
    if(m_ptrClient->bConnected==false&&m_ptrClient->bConnectionTransversed==true)
    {
        QMessageBox Msgbox;
        Msgbox.setText("server not alive!");
        Msgbox.exec();
        return;
    }



    QStringList users;
    users<<strUser;
    ui->listWidget->clear();
    foreach(QString user, users)
    ui->listWidget->addItem(new QListWidgetItem(QPixmap(":/new/prefix1/user.png"), user));
    ui->stackedWidget->setCurrentWidget(ui->page_2);
    std::thread t([this](){this-> io_service.run(); });
    t.detach();
    connect(m_ptrClient.get(), SIGNAL(messageArrive(int)),
            this, SLOT(readMessage(int)));
    chat_message msg;
    msg.body_length(std::strlen(c_strUser));
    std::memcpy(msg.body(), c_strUser, msg.body_length());
    msg.encode_header();
    m_ptrClient->write(msg);
}

void MainWindow::readMessage(int str)
{
   // int i=iVal;
    std::lock_guard<std::mutex> lk(mu);
    QString strAppend="Server:";
    QString strQ=QString::fromStdString(g_str);
    strAppend+=strQ;
    ui->textEdit_3->append(strAppend);
    g_str.clear();

}
void MainWindow::on_pushButton_clicked()
{
    QString strLineInput=ui->lineEdit->text();
    QString strAppend;
    if(m_bUserSaid==false)
    {
        m_strUserName.append(":");
        m_bUserSaid=true;
    }
    strAppend=QString::fromStdString(m_strUserName);//"Me:";
    strAppend+=strLineInput;
    ui->textEdit_3->append(strAppend);
     QByteArray ba = strLineInput.toLatin1();
     const char* c_strInput=ba.data();
     chat_message msg;
   msg.body_length(std::strlen(c_strInput));
   std::memcpy(msg.body(), c_strInput, msg.body_length());
   msg.encode_header();
     m_ptrClient->write(msg);
}
