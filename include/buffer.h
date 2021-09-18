
#ifndef BUFFER_H_
#define BUFFER_H_
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <shared_mutex>

#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>


#include "constant.h"

#include "pc_strategy.hpp"
#include "mystrct.hpp"

class PCBuffer {
 public:
    static PCBuffer &get_instance() {
        static PCBuffer instance;
        return instance;
    }

    virtual ~PCBuffer() {}

    template <class T> void log(T s) {
        time_t now = time(0);
        tm *gmtm = gmtime(&now);
        std::lock_guard<std::mutex> lck(log_mutex_);
        std::ofstream outfile("bufferlog.txt", std::ios::out | std::ios::app);
        std::string time;
        time = format("%d.%d %d:%d ",gmtm->tm_year+1900,gmtm->tm_mday,(gmtm->tm_hour+8)%24,gmtm->tm_min);
        while(time.size()<14){
            time += ' ';
        }
        outfile << time;
        outfile << s << std::endl;
        outfile.close();
    }

    bool get_resource_info(std::string url ,std::shared_ptr<ResourceInfo> &ri ,bool flag_not_add_visit_times = false); // Get resource from net or buf ,return ResourceInfo

    void add_to_train_set(std::string url,int client); // 向测试集添加数据

    bool delete_resource_by_url(std::string url);

    bool delete_resource_by_ri(std::shared_ptr<ResourceInfo>);

    void clear_resource();

    bool add_resource(std::shared_ptr<ResourceInfo> ri);

    size_t get_max_size();

    size_t get_current_size(); // used size

    size_t get_cached_number();

    std::string get_strategy();

    std::unordered_map<std::string, std::shared_ptr<ResourceInfo>>& get_resource_map();

 private:

    bool get_resource_from_http_2file(const std::string & server, const std::string & path, size_t * size);

    bool get_resource_from_https_2file(const std::string & server, const std::string & path, size_t * size);

    PCBuffer();

    PCBuffer(const PCBuffer &rhs) = delete;

    PCBuffer &operator=(const PCBuffer &rhs) = delete;

    bool initialized_{false};
    bool quit_flag_{false};
    std::condition_variable cv_;
    std::mutex log_mutex_;
    std::mutex train_set_mutex_;
    std::shared_mutex buffer_mutex_;
    size_t current_size_{0}; // byte
    size_t max_size_{MAX_BUFFER_SIZE};
    std::unordered_map<std::string, std::shared_ptr<ResourceInfo>> resource_map_;
    std::shared_ptr<BaseStrategy>strategy_;
    //BaseStrategy* strategy_{nullptr};
};


#endif // BUFFER_H_

/*
todo 
delete resource file or release memory
api:when and where call the precached strategy
api:how to move resource to client part
put resource in memory
where to use thread pool
*/