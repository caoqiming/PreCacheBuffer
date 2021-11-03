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
	// ���û���ʼ����֮ǰ�ͻ�����Щ���ݣ�Ĭ�ϲ������κ����� ����ռ䲻�������Ȼ���urls�￿ǰ������

	virtual bool buffer_new_resource(std::shared_ptr<ResourceInfo> ri, size_t current_size,size_t max_size) = 0; //�Ƿ񻺴�����Դ

	virtual void buffered_resource_visited(std::shared_ptr<ResourceInfo> ri) = 0; //�Ѿ��������Դ�������˾͵��øú�������Ϊ����Ӱ���������

	template <class T> 
	bool clear_buffer_for_size(size_t size, T* obp, bool (T::* pf)(std::shared_ptr<ResourceInfo> ri)){
		//�������ָ�����������ɾ����Դ�ĺ���ָ�� ��ô����Ϊ�˱���ѭ������
		std::vector<std::shared_ptr<ResourceInfo>> delete_queue;
		clear_buffer_for_size_child(size,delete_queue);
		for(auto &it:delete_queue){
			(obp->*pf)(it);
		}
		return true;
	}

	//����Ҫɾ���Ĵ�С ����ѡ����Ҫɾ������Դ��ResourceInfo
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
	int timed_task_interval{60*10};//��ʱ����ִ�еļ�� ��λ��
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

	void timed_task() {
		//std::cout << "PCStrategyTime �Ķ�ʱ����"  << "\n";
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
		std::string server="127.0.0.1",port="8888",request_body,python_response_body, path="/model"; //����pathû�� ���ǲ���Ϊ��
		boost::property_tree::ptree response_pt;
		response_pt.put<std::string>("type", "get_strategy");
		response_pt.put<int>("nums", 1100);//���� 3952 ����Ӱ �����������Ҫ���ݻ����С���޸�
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
		// ��ȡjson����б�
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
		long long signed_size = static_cast<long long>(size); //�����л����з�����������ȷ�ж�
	}

	void timed_task() {
		//std::cout << "PCStrategyTime �Ķ�ʱ����"  << "\n";
	}

private:


};




#endif // PC_STRATEGY_HPP_