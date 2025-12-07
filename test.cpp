#include "Array.h"
#include <vector>
#include <iostream>


int main() {
    Potato::Array<int> arr1;
    Potato::Array<int> arr2(2);
    std::vector<int> vec3 = {1,2,3,4,5,6,7,8,9};
    vec3.resize(10, 1.2);
    std::cout << "Goodbye, World!" << std::endl;
    return 0;
}