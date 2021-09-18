#include <iostream> // std::cout
#include "buffer.h"
#include "my_thread_pool.hpp"
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "server.hpp"

#include "boost/bind.hpp"
#include "boost/function.hpp"

int main(int argc, char* argv[])
{
    PCBuffer &bf=PCBuffer::get_instance();

    MyThreadPool& tp = MyThreadPool::get_instance();
    tp.init(10);

    try
    {
        // Initialise the server.
        http::server::server s("0.0.0.0", "80",".");

        // Run the server until stopped.
        s.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << "\n";
    }

    return 0;
}


