/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lcao <lcao@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:43:49 by lcao              #+#    #+#             */
/*   Updated: 2026/03/24 14:44:17 by lcao             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

Server::Server(int port, std::string pass)
{
    _pass = pass;
    _initSocket(port);
}

Server::~Server()
{

	for (size_t i = 0; i < v_fds.size(); ++i)
        close(v_fds[i].fd);
    std::cout << "Server closed" << std::endl;
}

int& Server::getSocket() { return m_socket; }
std::vector<struct pollfd>& Server::getFds() { return v_fds; }
std::string Server::getPass() const { return _pass; }

void	Server::_initSocket(int port)
{
    if ((m_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
        throw std::runtime_error("socket() error");

    int opt = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        throw std::runtime_error("setsockopt() error");

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(m_socket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
        throw std::runtime_error("bind() error");

    if (listen(m_socket, 10) == -1)
        throw std::runtime_error("listen() error");

    struct pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    v_fds.push_back(pfd);

    std::cout << "server is ready!" << std::endl;
}


std::pair<int, std::string> Server::acceptClient()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int client_fd;
    std::string hostName;

    client_fd = accept4(m_socket, (struct sockaddr*)&clientAddr, &clientLen, SOCK_NONBLOCK);
    if (client_fd == -1)
        throw std::runtime_error("Accepting new Client failed");
    hostName = inet_ntoa(clientAddr.sin_addr);

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    v_fds.push_back(pfd);

    std::cout << "New client fd=" << client_fd
              << " from " << hostName
              << ":" << ntohs(clientAddr.sin_port) << std::endl;

    return (std::make_pair(client_fd, hostName));
}