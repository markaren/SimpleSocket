
#ifndef SIMPLE_SOCKET_AVAILABLEPORTQUERY_HPP
#define SIMPLE_SOCKET_AVAILABLEPORTQUERY_HPP

#include <vector>

int getAvailablePort(int startPort, int endPort, const std::vector<int>& excludePorts = {});

#endif//SIMPLE_SOCKET_AVAILABLEPORTQUERY_HPP
