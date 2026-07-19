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
        //rprint green message to indicate the config file being used
        std::cout << "\033[32mUsing config file: " << filename << "\033[0m" << std::endl;
    } else {
        filename = "config/config.conf";
        std::cout << "\033[32mUsing default config file: " << filename << "\033[0m" << std::endl;
    }
    WebServ server;
    try {
        server.parsing(filename);
        server.setup();
        server.start();
        // server.displayConfigs();
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    // server.start();
}