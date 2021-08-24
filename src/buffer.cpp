#include "buffer.h"

#include "http_client.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

bool PCBuffer::add_resource(std::shared_ptr<ResourceInfo> ri) {
    if(ri==nullptr){
        log("add_resource invalid parameter");
        return false;
    }
    std::shared_lock<std::shared_mutex> lck(buffer_mutex_);
    if (ri->size + current_size_ > MAX_SIZE) {
        log(format("try to add_resource failed due to insufficient space, url: %s "
                   "data_size: %d ,max_size: %d",
                   ri->url.c_str(), ri->size, MAX_SIZE));
        return false;
    }
    if (resource_map_.find(ri->url) != resource_map_.end()) {
        log(format("try to add_resource already exists, url: %s", ri->url.c_str()));
        return true;
    }
    resource_map_.insert({ri->url, ri});
    current_size_ += ri->size;
    return true;
}

bool PCBuffer::delete_resource(std::string url) {
    std::unique_lock<std::shared_mutex> lck(buffer_mutex_);
    auto it = resource_map_.find(url);
    if (it == resource_map_.end()) {
        log(format("try to delete_resource that do not exist, url: %s", url.c_str()));
        return true;
    }

    resource_map_.erase(it);
    return true;
}

bool PCBuffer::get_resource_from_http_2file(const std::string &server,const std::string &path,size_t *size) {
    try {
        boost::asio::io_context io_context;
        HttpClient c(io_context, server, path, size);
        io_context.run();
        return true;
    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << "\n";
        return false;
    }
    return false;
}

bool PCBuffer::get_resource_info(std::string url ,std::shared_ptr<ResourceInfo> &ri ,bool flag_not_add_visit_times){
    auto it = resource_map_.find(url);
    if (it != resource_map_.end()) { // resource found in buffer
        it->second->visit_times++;
        ri = it->second;
        return true;
    }
    // get resource from net
    bool flag_ssl = false;
    if (url.substr(0, 4) == "http") {
        if (url[4] == 's') { // https
            flag_ssl = true;
            url = url.substr(8);
        } else { // http
            url = url.substr(7);
        }
    }
    std::string server, path;
    int index = url.find_first_of("/");
    server = url.substr(0, index);
    path = url.substr(index);
    if (server.empty() || path.empty()) {
        log(format("invalid url: %s", url.c_str()));
        return false;
    }
    std::shared_ptr<ResourceInfo> new_ri;
    if(flag_ssl){
        log(format("don't support https yet QAQ, url: %s", url.c_str()));
        return false;
    }else{
        size_t* size=new size_t;
        if(!get_resource_from_http_2file(server, path,size)){
            log(format("get_resource_from_http_2file failed, url: %s", url.c_str()));
            delete size;
            return false;
        }
        new_ri = std::make_shared<ResourceInfo>(*size,"http://"+url);
        delete size;
    }
    if(!add_resource(new_ri)){
        return false;
    }
    ri = new_ri;
    if(!flag_not_add_visit_times) // if it is preload flag_not_add_visit_times should be true 
        ri->visit_times++;
    return true;
}

