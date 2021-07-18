#include "buffer.h"

#include "util.hpp"
bool PCBuffer::get_resource_from_net(std::string url) { return false; }

bool PCBuffer::get_resource_from_buffer(std::string url) { return false; }

bool PCBuffer::add_resource(void *r, int data_size, std::string url) {
    std::lock_guard<std::mutex> lck(buffer_mutex_);
    if (data_size + current_size > MAX_SIZE) {
        log(format("try to add_resource failed due to insufficient space, url: %s "
                   "data_size: %d ,max_size: %d",
                   url.c_str(), data_size, MAX_SIZE));
        return false;
    }
    if (resource_map.find(url) != resource_map.end()) {
        log(format("try to add_resource already exists, url: %s", url.c_str()));
        return true;
    }
    auto ri = std::make_shared<ResourceInfo>(data_size, url);
    resource_map.insert({url, ri});
    current_size += data_size;
    return true;
}

bool PCBuffer::delete_resource(std::string url) {
    auto it = resource_map.find(url);
    if (it == resource_map.end()) {
        log(format("try to delete_resource not exists, url: %s", url.c_str()));
        return true;
    }
    //此处释放buffer的内存

    resource_map.erase(it);
    return true;
}
