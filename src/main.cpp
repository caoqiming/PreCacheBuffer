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
    

    MyThreadPool& tp = MyThreadPool::get_instance();
    tp.init(10);

    PCBuffer &bf=PCBuffer::get_instance();//buffer 里定时任务有用到线程池，所以必须先初始化线程池再初始化buffer

    while(bf.running_){
        try
        {
            // Initialise the server.
            http::server::server s("0.0.0.0", "80",".");

            // Run the server until stopped.
            s.run();
        }
        catch (std::exception& e)
        {
            std::cerr << "exception(main server): " << e.what() << "\n";
        }
    }


    return 0;
}


