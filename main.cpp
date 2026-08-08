#include <iostream>
#include "src/webserv.hpp"

int main(int ac, char **av){
    std::string filename;
    if(ac > 2) {
        std::cerr << "Error: Too many arguments" << std::endl;
        return 1;
    }
    if (ac == 2) {
        filename = av[1];
    } else {
        filename = "config/config.conf";
    }
    WebServ server;
    try {

        std::cout << "\033[34m[INFO] Parsing config file: " << filename << "\033[0m" << std::endl;
        server.parsing(filename);
        std::cout << "\033[34m[INFO] Loading saved tokens...\033[0m" << std::endl;
        server.loadTokens("cgi/users.json");
        std::cout << "\033[34m[INFO] Setting up sockets...\033[0m" << std::endl;
        server.setup();
        std::cout << "\033[32m[OK] webserv is ready.\033[0m" << std::endl;
        server.start();
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}