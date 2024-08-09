
#ifndef SIMPLE_SOCKET_PORT_QUERY_HPP
#define SIMPLE_SOCKET_PORT_QUERY_HPP

#include <vector>

int getAvailablePort(int startPort, int endPort, const std::vector<int>& excludePorts = {});

#endif//SIMPLE_SOCKET_PORT_QUERY_HPP
