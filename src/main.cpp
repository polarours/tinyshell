#include "shell.hpp"
#include <iostream>

int main() {
    try {
        tinyshell::Shell shell;
        shell.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
    
    return 0;
}
