
#ifndef HTTP_CLIENT_HPP_
#define HTTP_CLIENT_HPP_
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

using boost::asio::ip::tcp;

class HttpClient {
 public:
    ~HttpClient() { outfile_.close(); }
    HttpClient(boost::asio::io_context &io_context, const std::string &server, const std::string &path, size_t *size)
        : resolver_(io_context), socket_(io_context) {
        *size = static_cast<size_t>(0);
        size_ = size;
        outfile_.open("data/" + path.substr(path.find_last_of('/') + 1), std::ios::app | std::ios::binary);
        std::ostream request_stream(&request_);
        request_stream << "GET " << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << server << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";
        resolver_.async_resolve(server, "http",
                                boost::bind(&HttpClient::handle_resolve, this, boost::asio::placeholders::error,
                                            boost::asio::placeholders::results));
    }

 private:
    void handle_resolve(const boost::system::error_code &err, const tcp::resolver::results_type &endpoints) {
        if (!err) {
            // Attempt a connection to each endpoint in the list until we
            // successfully establish a connection.
            boost::asio::async_connect(
                    socket_, endpoints,
                    boost::bind(&HttpClient::handle_connect, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_connect(const boost::system::error_code &err) {
        if (!err) {
            // The connection was successful. Send the request.
            boost::asio::async_write(
                    socket_, request_,
                    boost::bind(&HttpClient::handle_write_request, this, boost::asio::placeholders::error));
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
                    boost::bind(&HttpClient::handle_read_status_line, this, boost::asio::placeholders::error));
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
                    boost::bind(&HttpClient::handle_read_headers, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err << "\n";
        }
    }

    void handle_read_headers(const boost::system::error_code &err) {
        if (!err) {
            std::istream response_stream(&response_);
            std::string header;
            while (std::getline(response_stream, header) && header != "\r") {
                // std::cout << header << "\n";
            }

            // Start reading remaining data until EOF.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient::handle_read_content, this, boost::asio::placeholders::error));
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
                    boost::bind(&HttpClient::handle_read_content, this, boost::asio::placeholders::error));
        } else if (err != boost::asio::error::eof) {
            std::cout << "Error: " << err << "\n";
        }
    }

    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    std::ofstream outfile_;
    size_t* size_;
};

#endif // HTTP_CLIENT_HPP_