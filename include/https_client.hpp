#ifndef HTTPS_CLIENT_HPP_ //并不是通用的，只是获取网络资源并保存到文件
#define HTTPS_CLIENT_HPP_

#include <cstdlib>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include "util.h"

class HttpsClient
{
public:
    ~HttpsClient(){ outfile_.close(); }
    HttpsClient(boost::asio::io_context& io_context,
        boost::asio::ssl::context& context,
        const std::string &server,const std::string &path,size_t resource_id,
        std::string &file_name,size_t *size
    )
    : socket_(io_context, context),server_(server),path_(path),resource_id_(resource_id),file_name_(file_name)
    {
        *size = static_cast<size_t>(0);
        size_ = size;
        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(server_, "443");
        socket_.set_verify_mode(boost::asio::ssl::verify_peer);
        socket_.set_verify_callback(boost::bind(&HttpsClient::verify_certificate, this,boost::placeholders::_1, boost::placeholders::_2));

        boost::asio::async_connect(socket_.lowest_layer(), endpoints,boost::bind(&HttpsClient::handle_connect, this,boost::asio::placeholders::error));
    }

  bool verify_certificate(bool preverified,boost::asio::ssl::verify_context& ctx)
  {
    return true;
  }

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_handshake(boost::asio::ssl::stream_base::client,
          boost::bind(&HttpsClient::handle_handshake, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Connect failed: " << error.message() << "\n";
    }
  }

  void handle_handshake(const boost::system::error_code& error)
  {
    if (!error)
    {
        std::ostream request_stream(&request_);
        request_stream << "GET " << path_ << " HTTP/1.0\r\n";
        request_stream << "Host: " << server_ << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        boost::asio::async_write(
                 socket_, request_,
                 boost::bind(&HttpsClient::handle_write, this ,boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Handshake failed: " << error.message() << "\n";
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
        if (!error)
        {
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpsClient::handle_read_headers, this, boost::asio::placeholders::error));
        }
        else
        {
            std::cout << "Write failed: " << error.message() << "\n";
        }
  }

    void handle_read_headers(const boost::system::error_code &err) {
        if (!err) {
            std::istream response_stream(&response_);
            std::string header;
            while (std::getline(response_stream, header) && header != "\r") {
                //get Content-Type
                if(header.substr(0,12)=="Content-Type"){
                    file_name_ =std::to_string(resource_id_)+"."+ header.substr(header.find_last_of('/') + 1);
                    deleteAllMark(file_name_,"\r");
                    outfile_.open("data/" + file_name_, std::ios::app | std::ios::binary);//path.substr(path.find_last_of('/') + 1)
                    if (!outfile_) {
                        std::cout << "打开文件失败,文件名:" <<file_name_<<"\n";
                    }
                }
            }

            // Start reading remaining data until EOF.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpsClient::handle_read_content, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err << "\n";
        }
    }

    void handle_read_content(const boost::system::error_code &err) {
        if (!err) {
            // Write all of the data that has been read so far.
            // std::cout << std::endl << "-------" << std::endl;
            // std::cout << &response_;
            *size_+=response_.size();
            outfile_ << &response_;
            // Continue reading remaining data until EOF.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpsClient::handle_read_content, this, boost::asio::placeholders::error));
        } else if (err != boost::asio::error::eof) {
            std::cout << "Error: " << err << "\n";
        }
    }

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    std::string server_;
    std::string path_;
    std::ofstream outfile_;
    size_t* size_;
    size_t resource_id_;
    std::string &file_name_;
};


#endif // HTTPS_CLIENT_HPP_