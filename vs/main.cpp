#include "app.h"
#include <iostream>

int main() {
    App app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}

