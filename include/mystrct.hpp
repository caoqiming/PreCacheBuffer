
#ifndef MY_STRUCT_HPP_
#define MY_STRUCT_HPP_
#include<string>
struct ResourceInfo {
    size_t size;           // 字节数量
    unsigned int visit_times;
    std::string url;
    bool is_in_memory;
    char *p;               // 如果在内存中 起始指针
    std::string file_name; // 如果在磁盘中 文件名
    ResourceInfo(size_t s, std::string u , std::string file_name) : size(s), url(u), visit_times(0), is_in_memory(false), p(nullptr),file_name(file_name){};
    ResourceInfo(size_t s, std::string u , char* p) : size(s), url(u), visit_times(0), is_in_memory(true), p(p){};
};




#endif // MY_STRUCT_HPP_
