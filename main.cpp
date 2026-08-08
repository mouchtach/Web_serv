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

        server.parsing(filename);
        server.loadTokens("cgi/users.json");
        server.setup();
        server.start();
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}