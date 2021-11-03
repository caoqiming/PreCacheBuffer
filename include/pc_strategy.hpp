#ifndef PC_STRATEGY_HPP_
#define PC_STRATEGY_HPP_
#include "mystrct.hpp"
#include<iostream>
#include<queue>
#include<vector>
#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/json_parser.hpp>
#include "http_client_to_python.hpp"
using std::cout;
using std::endl;

class BaseStrategy{
public:
	virtual ~BaseStrategy(){}

	virtual void precached_before_start(std::vector<std::string>&urls) {}
	// 在用户开始访问之前就缓存哪些内容，默认不缓存任何内容 如果空间不够，优先缓存urls里靠前的内容

	virtual bool buffer_new_resource(std::shared_ptr<ResourceInfo> ri, size_t current_size,size_t max_size) = 0; //是否缓存新资源

	virtual void buffered_resource_visited(std::shared_ptr<ResourceInfo> ri) = 0; //已经缓存的资源被访问了就调用该函数，因为可能影响后续策略

	template <class T> 
	bool clear_buffer_for_size(size_t size, T* obp, bool (T::* pf)(std::shared_ptr<ResourceInfo> ri)){
		//输入类的指针和类中用来删除资源的函数指针 这么做是为了避免循环引用
		std::vector<std::shared_ptr<ResourceInfo>> delete_queue;
		clear_buffer_for_size_child(size,delete_queue);
		for(auto &it:delete_queue){
			(obp->*pf)(it);
		}
		return true;
	}

	//输入要删除的大小 返回选出来要删掉的资源的ResourceInfo
	virtual void clear_buffer_for_size_child(size_t size,std::vector<std::shared_ptr<ResourceInfo>>&delete_queue) = 0;

	virtual void timed_task() {}

	std::string get_name() { 
		return name; 
	}

	int get_timed_task_interval(){
		return timed_task_interval;
	}

protected:
	std::string name;
	std::unordered_map<std::string, std::shared_ptr<ResourceInfo>>* resource_map_;
	int timed_task_interval{60*10};//定时任务执行的间隔 单位秒
};

class PCStrategyTime:public BaseStrategy{
public:
	PCStrategyTime(std::unordered_map<std::string, std::shared_ptr<ResourceInfo>>* resource_map){
		resource_map_ = resource_map;
		name = "PCStrategyTime";
		//初始化ri_queue_ 如果改策略启用之前buffer里已经有数据了则需要初始化，否则clear_buffer_for_size可能出问题
		if(!resource_map_->empty()){
			cout << "启动策略时资源map不为空，初始化PCStrategyTime的ri_queue_" << endl;
			for(const auto &it:*resource_map_){
				ri_queue_.push(it.second);//即使访问次数是0（真预缓存）也要加入队列
				//即访问次数0和1都加入一个，其他访问多少次加入多少个
				for(int j=1;j<it.second->visit_times;++j){
					ri_queue_.push(it.second);
				}
			}
		}
	}

	bool buffer_new_resource(std::shared_ptr<ResourceInfo> ri, size_t current_size,size_t max_size){
		ri_queue_.push(ri);
		return true;
	}

	void buffered_resource_visited(std::shared_ptr<ResourceInfo> ri){
		ri_queue_.push(ri);
	}

	void clear_buffer_for_size_child(size_t size, std::vector<std::shared_ptr<ResourceInfo>>&delete_queue){
		long long signed_size = static_cast<long long>(size); //必须切换到有符号数才能正确判断
		try
		{
			while (signed_size > 0) {
				while (true) {
					if(ri_queue_.empty()){
						std::cout << "clear_buffer_for_size_child error : 队列已清空但空间仍然不够！\n可能是初始化有问题或切换策略出现问题\n";
						return;
					}
					auto &ri_out = ri_queue_.front();
					ri_queue_.pop();
					if (--ri_out->visit_times <= 0) { //不用等于是因为真预缓存的访问次数本身就是0，减一之后为负
						signed_size -= ri_out->size;
						delete_queue.emplace_back(ri_out);
						break;
					}
				}
			}
		}
		catch (std::exception& e)
		{
			std::cout << "clear_buffer_for_size_child exception: " << e.what() << "\n";
		}
	}

	void timed_task() {
		//std::cout << "PCStrategyTime 的定时任务"  << "\n";
	}

private:
	std::queue<std::shared_ptr<ResourceInfo>>ri_queue_;

};

class PCStrategy_CF_AutoEncoder:public BaseStrategy{
public:
	PCStrategy_CF_AutoEncoder(std::unordered_map<std::string, std::shared_ptr<ResourceInfo>>* resource_map){
		resource_map_ = resource_map;
		name = "PCStrategy_CF_AutoEncoder";
	}

	void precached_before_start(std::vector<std::string> &urls) {
		std::string server="127.0.0.1",port="8888",request_body,python_response_body, path="/model"; //这里path没用 但是不能为空
		boost::property_tree::ptree response_pt;
		response_pt.put<std::string>("type", "get_strategy");
		response_pt.put<int>("nums", 1100);//共有 3952 个电影 这里请求多少要根据缓存大小来修改
		std::stringstream wos;
		boost::property_tree::write_json(wos, response_pt);
		request_body = wos.str();
		try {
			boost::asio::io_context io_context;
			HttpClient2Python c(io_context, server,port, path, request_body,python_response_body);
			io_context.run();
		} catch (std::exception &e) {
			std::cout << "Exception: " << e.what() << "\n";
		}
		// std::cout << python_response_body;
		auto pt = std::make_shared<boost::property_tree::ptree>();
		if(!json_decode(python_response_body,pt)){
			cout<< "error: python_response_body is not json"<<endl;
			return;
		}
		if(pt->find("rank")==pt->not_found()){
			cout<< "error: python_response_body key \"rank\" not found"<<endl;
			return;
		}
		// 读取json里的列表
		boost::property_tree::ptree rank_array = pt->get_child("rank"); 
		boost::property_tree::ptree::iterator pos = rank_array.begin();
		for(; pos != rank_array.end(); ++pos)
		{
		   urls.emplace_back(pos->second.get_value<std::string>());
		}
	}

	bool buffer_new_resource(std::shared_ptr<ResourceInfo> ri, size_t current_size,size_t max_size){
		return false;
	}

	void buffered_resource_visited(std::shared_ptr<ResourceInfo> ri){
	}

	void clear_buffer_for_size_child(size_t size, std::vector<std::shared_ptr<ResourceInfo>>&delete_queue){
		long long signed_size = static_cast<long long>(size); //必须切换到有符号数才能正确判断
	}

	void timed_task() {
		//std::cout << "PCStrategyTime 的定时任务"  << "\n";
	}

private:


};




#endif // PC_STRATEGY_HPP_