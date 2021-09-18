#ifndef BUFFER_HANDLER_HPP
#define BUFFER_HANDLER_HPP
#include <iostream>
#include "reply.hpp"
#include "request.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <boost/foreach.hpp>
#include <string>
#include "buffer.h"
#include <util.h>

namespace http {
namespace server {

	void get_resource_info_handler(std::shared_ptr<boost::property_tree::ptree>pt,std::string &response_body){
		// check request
		if(pt->find("url")==pt->not_found()){
			response_body = "{\"error\":\"key: url not found\"}";
			return;
		}
		if(pt->find("client")==pt->not_found()){
			response_body = "{\"error\":\"key: client not found\"}";
			return;
		}
		std::string url = pt->get<std::string>("url");
		int client=pt->get<int>("client");
		PCBuffer &bf=PCBuffer::get_instance();
		bf.add_to_train_set(url,client);
		std::shared_ptr<ResourceInfo> ri;
		if(bf.get_resource_info( url, ri)){
			boost::property_tree::ptree response_pt;
			response_pt.put<std::string>("file_name", ri->file_name);
			response_pt.put<int>("status", 0);
			std::stringstream wos;
			boost::property_tree::write_json(wos, response_pt);
			response_body = wos.str();
		}
		else{ // failed to get resource
			boost::property_tree::ptree response_pt;
			response_pt.put<int>("status", 1);
			response_pt.put<std::string>("error", "get this url failed");
			std::stringstream wos;
			boost::property_tree::write_json(wos, response_pt);
			response_body = wos.str();
		}
	}

	void get_buffer_status_handler(std::shared_ptr<boost::property_tree::ptree>pt,std::string &response_body){

		PCBuffer &bf=PCBuffer::get_instance();

		boost::property_tree::ptree response_pt;
		response_pt.put<size_t>("buffer_max_size",bf.get_max_size());
		response_pt.put<size_t>("buffer_current_size", bf.get_current_size());
		response_pt.put<size_t>("current_cached_number", bf.get_cached_number());
		response_pt.put<int>("status", 0);
		std::stringstream wos;
		boost::property_tree::write_json(wos, response_pt);
		response_body = wos.str();
	}

	void clear_buffer_handler(std::shared_ptr<boost::property_tree::ptree>pt,std::string &response_body){
		std::cout <<"warning: start clear buffer"<<std::endl;
		PCBuffer &bf=PCBuffer::get_instance();
		bf.clear_resource();

		boost::property_tree::ptree response_pt;
		response_pt.put<int>("status", 0);
		std::stringstream wos;
		boost::property_tree::write_json(wos, response_pt);
		response_body = wos.str();
	}

	void get_strategy_handler(std::shared_ptr<boost::property_tree::ptree>pt,std::string &response_body){
		PCBuffer &bf=PCBuffer::get_instance();
		boost::property_tree::ptree response_pt;
		response_pt.put<int>("status", 0);
		response_pt.put<std::string>("strategy", bf.get_strategy());
		std::stringstream wos;
		boost::property_tree::write_json(wos, response_pt);
		response_body = wos.str();
	}

	void router(std::string path, const request& req, reply& rep) {

		//std::cout <<"request debug:"<<std::endl;
		//std::cout << req.body<<std::endl;
		rep.status = reply::ok;
		bool flag_keep_going = true;
		std::string response_body;
		//std::cout<<req.uri<<std::endl;
		auto pt = std::make_shared<boost::property_tree::ptree>();
		if(!json_decode(req.body,pt)){
			response_body = "{\"error\":\"request body is not json\"}";
			flag_keep_going = false;
		}

		if(flag_keep_going && pt->find("type")==pt->not_found()){
			response_body = "{\"error\":\"key: type not found\"}";
			flag_keep_going = false;
		}

		if(flag_keep_going){
			std::string type = pt->get<std::string>("type");
			if(type=="get_resource_info"){
				get_resource_info_handler(pt,response_body);
			}
			else if(type=="get_buffer_status"){
				get_buffer_status_handler(pt,response_body);
			}
			else if(type=="clear_buffer"){
				clear_buffer_handler(pt,response_body);
			}
			else if(type=="get_strategy"){
				get_strategy_handler(pt,response_body);
			}
			else{
			response_body = "{\"error\":\"unkonwn type\"}";
			}
		}


		rep.content.append(response_body.c_str(),response_body.size());
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = mime_types::extension_to_type("json");

		//std::cout <<"response debug:"<<std::endl;
		//std::cout << response_body <<std::endl;
	}

}
}

#endif // BUFFER_HANDLER_HPP