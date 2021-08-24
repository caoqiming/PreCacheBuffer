
#ifndef BUFFER_H_
#define BUFFER_H_
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <ctime>
#include "constant.h"
#include "util.hpp"

struct ResourceInfo {
    size_t size;
    unsigned int visit_times;
    std::string url;
    bool is_in_memory;
    char *p;
    ResourceInfo(size_t s, std::string u) : size(s), url(u), visit_times(0), is_in_memory(false), p(nullptr){};
    ResourceInfo(size_t s, std::string u ,char* p) : size(s), url(u), visit_times(0), is_in_memory(true), p(p){};
};

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


 private:

    bool add_resource(std::shared_ptr<ResourceInfo> ri);

    bool delete_resource(std::string url);

    bool get_resource_from_http_2file(const std::string & server, const std::string & path, size_t * size);

    PCBuffer() {}
    PCBuffer(const PCBuffer &rhs) = delete;
    PCBuffer &operator=(const PCBuffer &rhs) = delete;
    bool initialized_{false};
    bool quit_flag_{false};
    std::condition_variable cv_;
    std::mutex log_mutex_;
    std::shared_mutex buffer_mutex_;
    size_t current_size_{0}; // byte
    int MAX_SIZE{MAX_BUFFER_SIZE};
    std::unordered_map<std::string, std::shared_ptr<ResourceInfo>> resource_map_;
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