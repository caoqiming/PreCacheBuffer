
#ifndef HTTP_CLIENT_TO_PYTHON_HPP_ //并不是通用的，只是获取网络资源并保存到文件
#define HTTP_CLIENT_TO_PYTHON_HPP_
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <util.h>

using boost::asio::ip::tcp;

class HttpClient2Python {
 public:
    ~HttpClient2Python() {}
    HttpClient2Python(boost::asio::io_context &io_context, 
        const std::string &server,const std::string &port, const std::string &path,const std::string &request_body,std::string &body
    )
        : resolver_(io_context), socket_(io_context),body_(body)
    {

        std::ostream request_stream(&request_);
        request_stream << "GET " << path << " HTTP/1.1\r\n";
        request_stream << "Host: " << server << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Content-Length:"<<request_body.size()<<"\r\n";
        request_stream << "Connection:Close\r\n\r\n";
        request_stream << request_body;
        //request_stream << "";
        boost::asio::ip::tcp::resolver::query query(server, port);
        resolver_.async_resolve(query,
                                boost::bind(&HttpClient2Python::handle_resolve, this, boost::asio::placeholders::error,
                                            boost::asio::placeholders::results));
    }

 private:
    void handle_resolve(const boost::system::error_code &err, const tcp::resolver::results_type &endpoints) {
        if (!err) {
            // Attempt a connection to each endpoint in the list until we
            // successfully establish a connection.
            boost::asio::async_connect(
                    socket_, endpoints,
                    boost::bind(&HttpClient2Python::handle_connect, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_connect(const boost::system::error_code &err) {
        if (!err) {
            // The connection was successful. Send the request.
            boost::asio::async_write(
                    socket_, request_,
                    boost::bind(&HttpClient2Python::handle_write_request, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_write_request(const boost::system::error_code &err) {
        if (!err) {
            // Read the response status line. The response_ streambuf will
            // automatically grow to accommodate the entire line. The growth may
            // be limited by passing a maximum size to the streambuf
            // constructor.
            boost::asio::async_read_until(
                    socket_, response_, "\r\n",
                    boost::bind(&HttpClient2Python::handle_read_status_line, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_read_status_line(const boost::system::error_code &err) {
        if (!err) {
            // Check that response is OK.
            std::istream response_stream(&response_);
            std::string http_version;
            response_stream >> http_version;
            unsigned int status_code;
            response_stream >> status_code;
            std::string status_message;
            std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
                std::cout << "Invalid response\n";
                return;
            }
            if (status_code != 200) {
                std::cout << "Response returned with status code ";
                std::cout << status_code << "\n";
                return;
            }
            // Read the response headers, which are terminated by a blank line.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient2Python::handle_read_headers, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error1: " << err << "\n";
        }
    }

    void handle_read_headers(const boost::system::error_code &err) {
        if (!err) {
            std::istream response_stream(&response_);
            std::string header;
            while (std::getline(response_stream, header) && header != "\r") {
                //get Content-Type
            }

            // Start reading remaining data until EOF.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient2Python::handle_read_content, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error2: " << err << "\n";
        }
    }

    void handle_read_content(const boost::system::error_code &err) {
        if (!err || (err == boost::asio::error::eof && response_.size())) {
            // Write all of the data that has been read so far.
            // std::cout << std::endl << "-------" << std::endl;
            // std::cout << &response_;
            // Continue reading remaining data until EOF.
            body_+=std::string(boost::asio::buffers_begin(response_.data()), boost::asio::buffers_end(response_.data()));
            response_.consume(response_.size());// 清空buf
            if (err)
                return;
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient2Python::handle_read_content, this, boost::asio::placeholders::error));
        } else if(err != boost::asio::error::eof){
            std::cout << "Error3: " << err << "\n";
        }
    }

    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    std::string &body_;
};

#endif // HTTP_CLIENT_TO_PYTHON_HPP_
