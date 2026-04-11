#include "core.hpp" 

#include <leaf/core/error.hpp>

#include <iostream>

namespace lf::vk {
    error init(int argc, char* argv[]) {
        std::cout << "Hello vulkan\n";
        return error();
        
        

    }
    void exit() {

    }
}