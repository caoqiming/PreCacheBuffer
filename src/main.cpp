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


class CCommunicationMap
{
public:
    static CCommunicationMap &get_instance() {
        static CCommunicationMap instance;
        return instance;
    }

    void TestCommand( int nParam){
        std::cout << nParam;
    }
};




int main0(){
    CCommunicationMap &m_communication=CCommunicationMap::get_instance();
    auto fun =  boost::bind(&CCommunicationMap::TestCommand, &m_communication, 22);
    fun();
    return 0;
}





int debug(){
    PCBuffer &bf=PCBuffer::get_instance();
    std::shared_ptr<ResourceInfo> ri;
    bf.get_resource_info("https://img2.baidu.com/it/u=1506121011,1888356275&fm=26&fmt=auto&gp=0.jpg", ri);
    //bf.clear_resource();
    bf.delete_resource("https://img2.baidu.com/it/u=1506121011,1888356275&fm=26&fmt=auto&gp=0.jpg");
    std::cout << "ok";
    return 0;
}