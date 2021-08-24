#include <iostream> // std::cout
#include "buffer.h"

int main(){
    PCBuffer &bf=PCBuffer::get_instance();
    std::shared_ptr<ResourceInfo> ri;
    bf.get_resource_info("http://www.bupt.edu.cn/index.html", ri);

    std::cout << "ok";
    return 0;
}