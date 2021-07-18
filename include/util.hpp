
#ifndef UTIL_HPP_
#define UTIL_HPP_
#include <string>
template <typename... Args> static std::string format(const std::string &format, Args... args) {
    auto size_buf = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    std::unique_ptr<char[]> buf(new (std::nothrow) char[size_buf]);

    if (!buf)
        return std::string("");

    std::snprintf(buf.get(), size_buf, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size_buf - 1);
}

template <typename T> inline void safe_delete_void_ptr(void *&target) {
    if (nullptr != target) {
        T *temp = static_cast<T *>(target);
        delete temp;
        temp = nullptr;
        target = nullptr;
    }
}

#endif // UTIL_HPP_
