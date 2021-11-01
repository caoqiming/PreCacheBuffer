#include "buffer.h"
#include "util.h"
#include "http_client.hpp"
#include "https_client.hpp"
#include <ctime>
#include <queue>
#include "my_thread_pool.hpp"

PCBuffer::PCBuffer(){
    //boost::filesystem::path tmpPath("dataset/trainset.txt");
    //boost::filesystem::remove(tmpPath);
    //读取配置文件
    std::ifstream  config_file;
    config_file.open("config.json", std::ios::in);
    std::stringstream buffer;  
    buffer << config_file.rdbuf();  
    std::string config(buffer.str());
    config_file.close();
	auto pt = std::make_shared<boost::property_tree::ptree>();
	if(!json_decode(config,pt)){
        std::cerr << "读取配置文件config.json出错\n";
        return;
	}
    if(pt->find("strategy")!=pt->not_found()){
        switch(pt->get<int>("strategy")){
        case StrategyByTime:
            strategy_ = std::make_shared<PCStrategyTime>(&resource_map_);//&resource_map_
            //strategy_ = new PCStrategyTime(&resource_map_);
            break;
        case StrategyByCFAE:
            strategy_ = std::make_shared<PCStrategy_CF_AutoEncoder>(&resource_map_);
            break;
        default:
            strategy_ = std::make_shared<PCStrategyTime>(&resource_map_);
            //strategy_ = new PCStrategyTime(&resource_map_);
        }
    }
    if(pt->find("max_size")!=pt->not_found()){
        max_size_=pt->get<size_t>("max_size");
    }
    running_ = true;
    MyThreadPool& tp = MyThreadPool::get_instance();
    boost::asio::post(*tp.get_pool(), boost::bind(&PCBuffer::time_task_handler,this));//启动定时任务
    //初始化缓存 在用户开始访问之前先缓存一部分内容
    std::vector<std::string>urls;
    strategy_->precached_before_start(urls);
	std::shared_ptr<ResourceInfo> ri;
    for( std::string &url:urls){
        //预缓存
        cout << url << endl;
        boost::asio::post(*tp.get_pool(), boost::bind(&PCBuffer::get_resource_info,this,url, ri,true));
    }
}

bool PCBuffer::add_resource(std::shared_ptr<ResourceInfo> ri) {
    if(ri==nullptr){
        log("add_resource invalid parameter");
        return false;
    }
    std::shared_lock<std::shared_mutex> lck(buffer_mutex_);
    if (ri->size + current_size_ > max_size_) {
        log(format("try to add_resource failed due to insufficient space, url: %s "
                   "data_size: %d ,max_size: %d",
                   ri->url.c_str(), ri->size, max_size_));
        std::cerr << "warining: add resource failed!\n";
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

size_t PCBuffer::get_max_size()
{
    return max_size_;
}

size_t PCBuffer::get_current_size()
{
    return current_size_;
}

size_t PCBuffer::get_cached_number()
{
    return resource_map_.size();
}

std::string PCBuffer::get_strategy()
{
    return strategy_->get_name();
}

std::unordered_map<std::string, std::shared_ptr<ResourceInfo>>& PCBuffer::get_resource_map()
{
    return resource_map_;
}

bool PCBuffer::delete_resource_by_url(std::string url) {
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

bool PCBuffer::delete_resource_by_ri(std::shared_ptr<ResourceInfo> ri)
{
    current_size_ -= ri->size;
    if(ri->is_in_memory && ri->p){
        delete[] ri->p;
    }
    if(!ri->is_in_memory && !ri->file_name.empty()){
        boost::filesystem::path tmpPath("./data/"+ri->file_name);
        boost::filesystem::remove(tmpPath);
    }
    auto it = resource_map_.find(ri->url);
    if(it!=resource_map_.end()){
        resource_map_.erase(it);
    }
    return true;
}

void PCBuffer::clear_resource(){
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

bool PCBuffer::get_resource_from_http_2file(const std::string &server,const std::string &path, std::string &file_name,size_t *size) {
    try {
        boost::asio::io_context io_context;
        HttpClient c(io_context, server, path, ++resource_id_, file_name, size);
        io_context.run();
        return true;
    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << "\n";
        return false;
    }
    return false;
}

bool PCBuffer::get_resource_from_https_2file(const std::string& server, const std::string& path,std::string &file_name, size_t* size)
{
    try {
        boost::asio::io_context io_context;
        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
        ctx.set_default_verify_paths();
        HttpsClient c(io_context, ctx,server,path,++resource_id_,file_name,size);
        io_context.run();
        return true;
    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << "\n";
        return false;
    }
    return false;
}

void PCBuffer::time_task_handler()
{
    time_t time_now,time_start;//精确到秒
    time_start = time(NULL);//起始时间
    time_now = 0;//经过了的时间
    while(running_){
        while(time_now<strategy_->get_timed_task_interval()){
            Sleep(1000);
            time_now = time(NULL) - time_start;
        }
        strategy_->timed_task();
        //执行完任务后重置计时
        time_start = time(NULL);
        time_now = 0;
    }

}

PCBuffer::~PCBuffer()
{
    running_ = false;
}

bool PCBuffer::get_resource_info(std::string url ,std::shared_ptr<ResourceInfo> &ri ,bool flag_not_add_visit_times){
    auto it = resource_map_.find(url);
    if (it != resource_map_.end()) { // resource found in buffer
        it->second->visit_times++;
        ri = it->second;
        strategy_->buffered_resource_visited(ri);
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
    if(index==-1){
        log(format("invalid url: %s", url.c_str()));
        return false;
    }
    server = url.substr(0, index);
    path = url.substr(index);
    if (server.empty() || path.empty()) {
        log(format("invalid url: %s", url.c_str()));
        return false;
    }
    std::shared_ptr<ResourceInfo> new_ri;
    if(flag_ssl)
    {
        size_t* size=new size_t;
        std::string file_name;
        if(!get_resource_from_https_2file(server, path,file_name,size)){
            log(format("get_resource_from_https_2file failed, url: %s", url.c_str()));
            delete size;
            return false;
        }
        new_ri = std::make_shared<ResourceInfo>(*size,"https://"+url,file_name);
        delete size;
    }
    else
    {
        size_t* size=new size_t;
        std::string file_name;
        if(!get_resource_from_http_2file(server, path,file_name,size)){
            log(format("get_resource_from_http_2file failed, url: %s", url.c_str()));
            delete size;
            return false;
        }
        new_ri = std::make_shared<ResourceInfo>(*size,"http://"+url,file_name);
        delete size;
    }

    if (strategy_->buffer_new_resource(new_ri,current_size_,max_size_)) {//判断是否缓存
        //如果要缓存则清除出足够的空间 清除的策略也要用strategy_里的
        long long need_more_size =  new_ri->size + current_size_ - max_size_;
        if(need_more_size>0){
            strategy_->clear_buffer_for_size(static_cast<size_t>(need_more_size), this, &PCBuffer::delete_resource_by_ri);
        }
        if(!add_resource(new_ri)){
            boost::filesystem::path tmpPath("./data/"+new_ri->file_name);//缓存失败（一般是由于空间限制）就删除已经下载的文件
            boost::filesystem::remove(tmpPath);
            return false; 
        }
    }
    else{
        boost::filesystem::path tmpPath("./data/"+new_ri->file_name);//不缓存就删除已经下载的文件
        boost::filesystem::remove(tmpPath);
    }

    ri = new_ri;
    if(!flag_not_add_visit_times) // if it is preload flag_not_add_visit_times should be true 
        ri->visit_times++;
    return true;
}

void PCBuffer::add_to_train_set(std::string url, int client){
    std::lock_guard<std::mutex> lck(train_set_mutex_);
    std::ofstream outfile("dataset/trainset.txt", std::ios::out | std::ios::app);
    //outfile << format("{\"url\":\"%s\",\"client\":%d}",url,client) << std::endl;
    outfile << url << std::endl;
    outfile.close();
}

