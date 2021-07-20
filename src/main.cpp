#include <iostream> // std::cout
#include "buffer.h"

int main(){
    PCBuffer &bf=PCBuffer::get_instance();
    std::shared_ptr<ResourceInfo> ri;
    bf.get_resource_info("http://i0.hdslb.com/bfs/feed-admin/2453111b02990329a31d3a53091fd24fe630f93a.jpg", ri);

    std::cout << "ok";
    return 0;
}