#ifndef MY_THREAD_POOL_HPP_
#define MY_THREAD_POOL_HPP_
#include <algorithm>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <mutex>


class MyThreadPool {
 public:
    static MyThreadPool &get_instance() {
        static MyThreadPool instance;
        return instance;
    }

    virtual ~MyThreadPool() {}

    template <class T> void log(T s) {
        time_t now = time(0);
        tm *gmtm = gmtime(&now);
        std::ofstream outfile("thread pool log.txt", std::ios::out | std::ios::app);
        std::string time;
        time = format("%d.%d %d:%d ",gmtm->tm_year+1900,gmtm->tm_mday,(gmtm->tm_hour+8)%24,gmtm->tm_min);
        while(time.size()<14){
            time += ' ';
        }
        outfile << time;
        outfile << s << std::endl;
        outfile.close();
    }

    void init(int nums){
        pool_ = std::make_unique<boost::asio::thread_pool>(nums);
    }

    std::unique_ptr<boost::asio::thread_pool> &get_pool(){
        return pool_;
    }

 private:

    MyThreadPool() {}

    MyThreadPool(const MyThreadPool &rhs) = delete;

    MyThreadPool &operator=(const MyThreadPool &rhs) = delete;

    bool initialized_{false};
    bool quit_flag_{false};
    std::condition_variable cv_;
    std::unique_ptr<boost::asio::thread_pool> pool_;


};


#endif // MY_THREAD_POOL_HPP_