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

namespace http {
namespace server {
	bool json_decode(std::string strResponse,std::shared_ptr<boost::property_tree::ptree>pt){
		try{
			std::stringstream sstream(strResponse);
			boost::property_tree::json_parser::read_json(sstream, *pt);
		}
		catch (std::exception& e){
			//std::cout << "exception: " << e.what() << "\n";
			return false;
		}
		return true;
	}

	void get_resource_info_handler(std::shared_ptr<boost::property_tree::ptree>pt,std::string &response_body){
		// check request
		if(pt->find("url")==pt->not_found()){
			response_body = "{\"error\":\"key: url not found\"}";
			return;
		}
		PCBuffer &bf=PCBuffer::get_instance();
		std::shared_ptr<ResourceInfo> ri;
		if(bf.get_resource_info( pt->get<std::string>("url"), ri)){
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


	void router(std::string path, const request& req, reply& rep) {
		// Fill out the reply to be sent to the client.
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
			else{
			response_body = "{\"error\":\"unkonwn type\"}";
			}
		}


		rep.content.append(response_body.c_str(), response_body.size());
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = mime_types::extension_to_type("json");

	}


}
}

#endif // BUFFER_HANDLER_HPP