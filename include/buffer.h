
#ifndef BUFFER_H_
#define BUFFER_H_
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>

struct ResourceInfo {
    int size;
    int visit_times;
    std::string url;
    ResourceInfo(int s, std::string u) : size(s), url(u), visit_times(0){};
};

class PCBuffer {
 public:
    static PCBuffer &get_instance() {
        static PCBuffer instance;
        return instance;
    }

    virtual ~PCBuffer() {}

    template <class T> void log(T s) {
        std::lock_guard<std::mutex> lck(mutex_);
        std::ofstream outfile("bufferlog.txt", std::ios::out | std::ios::app);
        outfile << s << std::endl;
        outfile.close();
    }

    bool get_resource_from_net(std::string url);

    bool get_resource_from_buffer(std::string url);

    bool add_resource(void *r, int data_size, std::string url);

    bool delete_resource(std::string url);

 private:
    PCBuffer() {}
    PCBuffer(const PCBuffer &rhs) = delete;
    PCBuffer &operator=(const PCBuffer &rhs) = delete;
    bool initialized_{false};
    bool quit_flag_{false};
    std::condition_variable cv_;
    std::mutex mutex_;
    std::mutex buffer_mutex_;
    int current_size{0}; // byte
    int MAX_SIZE{1024};
    std::unordered_map<std::string, std::shared_ptr<ResourceInfo>> resource_map;
};

#endif // BUFFER_H_