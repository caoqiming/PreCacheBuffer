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

	virtual bool buffer_new_resource(std::shared_ptr<ResourceInfo> ri, size_t current_size,size_t max_size) = 0; //�Ƿ񻺴�����Դ

	virtual void buffered_resource_visited(std::shared_ptr<ResourceInfo> ri) = 0; //�Ѿ��������Դ�������˾͵��øú�������Ϊ����Ӱ���������

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
		//��ʼ��ri_queue_ ����Ĳ�������֮ǰbuffer���Ѿ�������������Ҫ��ʼ��������clear_buffer_for_size���ܳ�����
		if(!resource_map_->empty()){
			cout << "��������ʱ��Դmap��Ϊ�գ���ʼ��PCStrategyTime��ri_queue_" << endl;
			for(const auto &it:*resource_map_){
				ri_queue_.push(it.second);//��ʹ���ʴ�����0����Ԥ���棩ҲҪ�������
				//�����ʴ���0��1������һ�����������ʶ��ٴμ�����ٸ�
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
		long long signed_size = static_cast<long long>(size); //�����л����з�����������ȷ�ж�
		try
		{
			while (signed_size > 0) {
				while (true) {
					if(ri_queue_.empty()){
						std::cout << "clear_buffer_for_size_child error : ��������յ��ռ���Ȼ������\n�����ǳ�ʼ����������л����Գ�������\n";
						return;
					}
					auto &ri_out = ri_queue_.front();
					ri_queue_.pop();
					if (--ri_out->visit_times <= 0) { //���õ�������Ϊ��Ԥ����ķ��ʴ����������0����һ֮��Ϊ��
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