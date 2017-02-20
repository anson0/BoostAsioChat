#ifndef CLIENTCLASS_H
#define CLIENTCLASS_H
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "chat_message.h"
#include <QObject>
#include <QMessageBox>
#include <mutex>
extern std::string g_str;
extern std::mutex mu;
using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

 class  chat_client:public QObject
{
Q_OBJECT
 public:
signals: void messageArrive(int str);
public:
  chat_client(boost::asio::io_service& io_service,
      tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service)
  {
      bConnected=false;
      bConnectionTransversed=false;
    do_connect(endpoint_iterator);
  }

  void write(const chat_message& msg)
  {
    io_service_.post(
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    io_service_.post([this]() { socket_.close(); });
  }

private:
  void do_connect(tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::async_connect(socket_, endpoint_iterator,
        [this](boost::system::error_code ec, tcp::resolver::iterator)
        {
            bConnectionTransversed=true;
          if (!ec)
          {

              bConnected=true;
            do_read_header();
          }
          else
          {
              QMessageBox Msgbox;
              Msgbox.setText(ec.message().c_str());
              Msgbox.exec();
              close();
          }
        });
  }

  void do_read_header()
  {
      boost::asio::async_read(socket_,
              boost::asio::buffer(read_msg_.data(), chat_message::header_length),
              [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
              QMessageBox Msgbox;
              Msgbox.setText(ec.message().c_str());
              Msgbox.exec();
            socket_.close();
          }
        });
  }

  void do_read_body()
  {
      boost::asio::async_read(socket_,
              boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
              [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {

            std::lock_guard<std::mutex> lk(mu);
            g_str=read_msg_.body();
            g_str.assign(read_msg_.data(),std::find(read_msg_.data(),read_msg_.data()+read_msg_.max_body_length+read_msg_.header_length,'\0'));
            //g_str+="(";
            //g_str+=std::to_string(read_msg_.body_length());
            //g_str+=")";
            emit messageArrive(0);

            do_read_header();
          }
          else
          {
              QMessageBox Msgbox;
              Msgbox.setText(ec.message().c_str());
              Msgbox.exec();
            socket_.close();
          }
          memset(read_msg_.data(),0,read_msg_.max_body_length+read_msg_.header_length);
        });
  }

  void do_write()
  {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
              QMessageBox Msgbox;
              Msgbox.setText(ec.message().c_str());
              Msgbox.exec();
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;

public:
  bool bConnected;
  bool bConnectionTransversed;
};
#endif // CLIENTCLASS_H
