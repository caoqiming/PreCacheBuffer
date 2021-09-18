#ifndef PC_STRATEGY_HPP_
#define PC_STRATEGY_HPP_
#include "mystrct.hpp"
#include<iostream>
#include<queue>
#include<vector>
using std::cout;
using std::endl;

class BaseStrategy{
public:
	virtual ~BaseStrategy(){}

	virtual bool buffer_new_resource(std::shared_ptr<ResourceInfo> ri, size_t current_size,size_t max_size) = 0; //是否缓存新资源

	virtual void buffered_resource_visited(std::shared_ptr<ResourceInfo> ri) = 0; //已经缓存的资源被访问了就调用该函数，因为可能影响后续策略

	template <class T> 
	bool clear_buffer_for_size(size_t size, T* obp, bool (T::* pf)(std::shared_ptr<ResourceInfo> ri)){
		std::vector<std::shared_ptr<ResourceInfo>> delete_queue;
		clear_buffer_for_size_child(size,delete_queue);
		for(auto &it:delete_queue){
			(obp->*pf)(it);
		}
		return true;
	}

	virtual void clear_buffer_for_size_child(size_t size,std::vector<std::shared_ptr<ResourceInfo>>&delete_queue) = 0;

	virtual void timed_task() {}

	std::string get_name() { 
		return name; 
	}

protected:
	std::string name;
	std::unordered_map<std::string, std::shared_ptr<ResourceInfo>>* resource_map_;
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
private:
	std::queue<std::shared_ptr<ResourceInfo>>ri_queue_;

};

#endif // PC_STRATEGY_HPP_