#include "util.h"

#include <boost/property_tree/json_parser.hpp>







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

