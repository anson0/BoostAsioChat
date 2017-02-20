//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <boost/asio.hpp>
#include "chat_message.hpp"
//#include <boost/thread.hpp>
#include <thread>
using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }
  std::set<chat_participant_ptr>& getParticipants(){return participants_;}
private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
      bWelcomed=false;
  }

  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this,self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
              if(bWelcomed==false)
              {
                  std::string str="Hello ";
                  str+=read_msg_.body();
                  str+=" ,you are join to server now!";
                  read_msg_.body_length(str.size());
                  std::memcpy(read_msg_.body(), str.c_str(), read_msg_.body_length());
                  read_msg_.encode_header();
                  deliver(read_msg_);

                  bWelcomed=true;
                  do_read_header();
              }
              else
              {
                  //room_.deliver(read_msg_);
                    deliver(read_msg_);
                    std::cout<<read_msg_.data()<<std::endl;
                    do_read_header();

              }
              memset(read_msg_.data(),0,read_msg_.max_body_length+read_msg_.header_length);


          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
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
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  bool bWelcomed;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : acceptor_(io_service, endpoint),
      socket_(io_service)
  {
    do_accept();
  }
  chat_room& getRoom(){return room_;}

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<chat_session>(std::move(socket_), room_)->start();

          }

          do_accept();
        });
  }


  tcp::acceptor acceptor_;
  tcp::socket socket_;
  chat_room room_;
};

//----------------------------------------------------------------------

int main(int argc, char* argv[])
{
  try
  {
//    if (argc < 2)
//    {
//      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
//      return 1;
//    }

    boost::asio::io_service io_service;

    std::list<chat_server> servers;
    for (int i = 1; i < 2; ++i)
    {
      //tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      tcp::endpoint endpoint(tcp::v4(), 1234);
      servers.emplace_back(io_service, endpoint);
    }

    std::thread t([&io_service](){ io_service.run();});
//std::thread t([&io_service](){ io_service.run(); });
    char line[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
//         char prompt[]="please input the session number:";
//        std::cout<<prompt;
//        int iSessionNumber;
//        std::cin>>iSessionNumber;
        int iSizeRoom=(*servers.begin()).getRoom().getParticipants().size();
        auto room=(*servers.begin()).getRoom();//.getParticipants().begin();
        //advance(session,iSizeRoom);
       // auto ptrSession=*session;

      chat_message msg;
      msg.body_length(std::strlen(line));
      std::memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      //chat_session* ptr=(*session).get();
      room.deliver(msg);
      //c.write(msg);
    }

  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
