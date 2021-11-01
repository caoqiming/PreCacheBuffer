#include "util.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>





void deleteAllMark(std::string &s, const std::string &mark)
{
	size_t nSize = mark.size();
	while(1)
	{
		size_t pos = s.find(mark);    //  尤其是这里
		if(pos == std::string::npos)
		{
			return;
		}
 
		s.erase(pos, nSize);
	}
}



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

