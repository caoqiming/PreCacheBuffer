#include "buffer.h"



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
    current_size_ -= it->second->size;
    if(it->second->is_in_memory && it->second->p){
        delete[] it->second->p;
    }
    if(!it->second->is_in_memory && !it->second->file_name.empty()){
        boost::filesystem::path tmpPath("./data/"+it->second->file_name);
        boost::filesystem::remove(tmpPath);
    }
    resource_map_.erase(it);
    return true;
}

bool PCBuffer::delete_resource(std::shared_ptr<ResourceInfo> ri)
{
    current_size_ -= ri->size;
    if(ri->is_in_memory && ri->p){
        delete[] ri->p;
    }
    if(!ri->is_in_memory && !ri->file_name.empty()){
        boost::filesystem::path tmpPath("./data/"+ri->file_name);
        boost::filesystem::remove(tmpPath);
    }
    auto it = resource_map_.find(ri->file_name);
    if(it!=resource_map_.end()){
        resource_map_.erase(it);
    }
    return true;
}

void PCBuffer::clear_resource()
{
    {
        std::unique_lock<std::shared_mutex> lck(buffer_mutex_);
        for(auto &it:resource_map_){
            if(it.second->is_in_memory && it.second->p){
                delete[] it.second->p;
            }
        }
        resource_map_.clear();
    }
    boost::filesystem::path tmpPath("./data");
    boost::filesystem::directory_iterator diter(tmpPath);
    boost::filesystem::directory_iterator diter_end;
    for (; diter != diter_end; ++diter) {
        if (!boost::filesystem::is_regular_file(diter->status())) continue;
        //std::string filename = diter->path().filename().string();
        //std::cout << filename << std::endl;
        boost::filesystem::remove(diter->path());
    }
    current_size_ = 0;
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

bool PCBuffer::get_resource_from_https_2file(const std::string& server, const std::string& path, size_t* size)
{
    try {
        boost::asio::io_context io_context;
        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
        ctx.set_default_verify_paths();
        HttpsClient c(io_context, ctx,server,path,size);
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
        size_t* size=new size_t;
        if(!get_resource_from_https_2file(server, path,size)){
            log(format("get_resource_from_https_2file failed, url: %s", url.c_str()));
            delete size;
            return false;
        }
        std::string file_name = path.substr(path.find_last_of('/') + 1);
        new_ri = std::make_shared<ResourceInfo>(*size,"https://"+url,file_name);
        delete size;
    }else{
        size_t* size=new size_t;
        if(!get_resource_from_http_2file(server, path,size)){
            log(format("get_resource_from_http_2file failed, url: %s", url.c_str()));
            delete size;
            return false;
        }
        std::string file_name = path.substr(path.find_last_of('/') + 1);
        new_ri = std::make_shared<ResourceInfo>(*size,"http://"+url,file_name);
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

