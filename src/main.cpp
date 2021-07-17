#include"buffer.h"
#include<iostream>
#include <boost/lambda/lambda.hpp>
#include <iterator>
#include <algorithm>

using namespace std;


int main() {
	PCBuffer &buffer=PCBuffer::get_instance();
    using namespace boost::lambda;
    typedef std::istream_iterator<int> in;

    std::for_each(
        in(std::cin), in(), std::cout << (_1 * 3) << " ");
    return 0;
}