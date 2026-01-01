#pragma once

#include <iostream>
#include <vector>
#include <exception>
#include <sstream>
#include <stdexcept>

#define LOG_INFO std::cerr << "[INFO] "
#define LOG_DEBUG std::cerr << "[DEBUG] "
#define LOG_ERR std::cerr << "[ERROR] "

extern volatile sig_atomic_t g_running;

unsigned short parsePort(const std::string& str);
void signal_handler(int sig);
