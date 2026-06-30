// #include "config/ParssingConf.hpp"
#include "Webserv.hpp"
#include <iostream>
// ./we    
int main(int argc, char **argv) {
    // signal(SIGPIPE, SIG_IGN);f

    if(argc == 1 || argc == 2)
    {
        std::string configFile ;
        if (argc == 1) {
            configFile = "config/default.conf";
        } else {
            configFile = argv[1];
        }
        Webserv  Webserv;
        try
        {
            Webserv.setupServers(configFile);
            Webserv.Start();
        }
        catch (const std::exception &e)
        {
            return 1;
        }   
    }
    else
    {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }
}