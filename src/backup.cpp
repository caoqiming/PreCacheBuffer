#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>

int main() {
    boost::system::error_code ec;
    using namespace boost::asio;

    // what we need
    io_service svc;
    ssl::context ctx(svc, ssl::context::method::sslv23_client);
    ssl::stream<ip::tcp::socket> ssock(svc, ctx);
    ssock.lowest_layer().connect({"www.boost.org", 443});
    ssock.handshake(ssl::stream_base::handshake_type::client);

    // send request
    std::string request(
            "GET /doc/libs/develop/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp HTTP/1.1\r\n\r\n");
    boost::asio::write(ssock, buffer(request));

    // read response
    std::string response;

    do {
        char buf[1024];
        size_t bytes_transferred = ssock.read_some(buffer(buf), ec);
        if (!ec)
            response.append(buf, buf + bytes_transferred);
    } while (!ec);

    // print and exit
    std::cout << "Response received: '" << response << "'\n";
}

#include <algorithm>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <mutex>
using namespace std;

void Task(int i, mutex *mutex_) {
    mutex_->lock();
    cout << "thread" << i << "complete" << endl;
    mutex_->unlock();
}

int main() {
    mutex mutex_;
    boost::asio::thread_pool pool(2);
    for (int i = 1; i < 10; ++i) {
        boost::asio::post(pool, boost::bind(Task, i, &mutex_));
    }
    pool.join();
}