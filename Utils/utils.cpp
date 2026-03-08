#include "utils.hpp"

std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}

unsigned short parsePort(const std::string &str)
{
    if (str.empty())
        throw std::invalid_argument("empty port");

    std::stringstream ss(str);
    long port;
    char extra;

    if (!(ss >> port) || (ss >> extra))
        throw std::invalid_argument("invalid port");

    if (port <= 1023 || port > 49151)
        throw std::invalid_argument("port out of allowed range");

    return static_cast<unsigned short>(port);
}

void signal_handler(int sig)
{
    if (sig == SIGINT)
        g_running = 0;
}
