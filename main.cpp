#include "./Utils/utils.hpp"
#include "./Server/Server.hpp"

volatile sig_atomic_t g_running = 1;

int main(int argc , char **argv)
{

    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }
    try 
    {
        unsigned short port = parsePort(argv[1]);
        Server server(port, argv[2]);
        std::signal(SIGINT, signal_handler);
        server.init();
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() <<std::endl;
        return 1;
    }
    return 0;
}