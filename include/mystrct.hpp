
#ifndef MY_STRUCT_HPP_
#define MY_STRUCT_HPP_
#include<string>
struct ResourceInfo {
    size_t size;           // �ֽ�����
    unsigned int visit_times;
    std::string url;
    bool is_in_memory;
    char *p;               // ������ڴ��� ��ʼָ��
    std::string file_name; // ����ڴ����� �ļ���
    ResourceInfo(size_t s, std::string u , std::string file_name) : size(s), url(u), visit_times(0), is_in_memory(false), p(nullptr),file_name(file_name){};
    ResourceInfo(size_t s, std::string u , char* p) : size(s), url(u), visit_times(0), is_in_memory(true), p(p){};
};




#endif // MY_STRUCT_HPP_
