/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Loop.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: suna <suna@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:54:37 by suna              #+#    #+#             */
/*   Updated: 2026/03/24 14:54:38 by suna             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>


void cleanupUserChannels(int fd, User &userManager, Channel &channelManager)
{
    struct userInfo &info = userManager.getUserInfo(fd);
    std::string prefix = ":" + info.nickName + "!" + info.loginName + "@" + info.hostName;
    std::string quitMsg = prefix + " QUIT :Connection closed\r\n";
    std::set<int> notified;

    std::set<std::string>::iterator it;
    for (it = info.channelList.begin(); it != info.channelList.end(); ++it)
    {
        std::string channelName = *it;
        if (channelManager.isExist(channelName))
        {
            std::set<int> &channelUsers = channelManager.getUsers(channelName);
            for (std::set<int>::iterator uIt = channelUsers.begin(); uIt != channelUsers.end(); ++uIt)
            {
                if (*uIt != fd && notified.find(*uIt) == notified.end())
                {
                    userManager.getUserInfo(*uIt).writeBuffer += quitMsg;
                    notified.insert(*uIt);
                }
            }
        }
        channelManager.removeUserFromChannel(*it, fd);
    }
    channelManager.removeFromAllInviteLists(fd);
}

bool handleClientPollIn(int fd, User &userManager, Channel &channelManager, std::vector<struct pollfd> &fds, size_t &index, std::string serverPass)
{
    char buffer[512];
    std::memset(buffer, 0, sizeof(buffer));

    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read == -1)
    {
        std::cout << "Client " << fd << " recv error." << std::endl;
        cleanupUserChannels(fd, userManager, channelManager);
        close(fd);
        userManager.removeUser(fd);
        fds.erase(fds.begin() + index);
        index--;
        return true;
    }
    if (bytes_read == 0)
    {
        std::cout << "Client " << fd << " disconnected." << std::endl;
        cleanupUserChannels(fd, userManager, channelManager);
        close(fd);
        userManager.removeUser(fd);
        fds.erase(fds.begin() + index);
        index--;
        return true;
    }

    std::string &readBuf = userManager.getUserInfo(fd).readBuffer;
    if (readBuf.size() + bytes_read > 512)
    {
        std::cout << "Client " << fd << " exceeded buffer limit. Disconnecting." << std::endl;
        std::string errMsg = "ERROR :Input buffer overflow\r\n";
        send(fd, errMsg.c_str(), errMsg.size(), 0);
        cleanupUserChannels(fd, userManager, channelManager);
        close(fd);
        userManager.removeUser(fd);
        fds.erase(fds.begin() + index);
        index--;
        return true;
    }

    userManager.handleClientData(fd, std::string(buffer), serverPass, channelManager);
    return false;
}

void handleClientPollOut(Server &irc, User &userManager, Channel &channelManager, size_t &i)
{
    std::vector<struct pollfd> &fds = irc.getFds();

    int fd = fds[i].fd;
    struct userInfo &info = userManager.getUserInfo(fd);
    std::string &wBuf = info.writeBuffer;

	if (!wBuf.empty())
	{
		int byte_sent = send(fd, wBuf.c_str(), wBuf.size(), 0);
		if (byte_sent > 0)
		{
			wBuf.erase(0, byte_sent);
        	if (wBuf.empty()) {
            	fds[i].events &= ~POLLOUT;
        }
		}
		else if (byte_sent == -1)
		{
			std::cout << "Error sending to client " << fd << ". Disconnecting." << std::endl;
			cleanupUserChannels(fd, userManager, channelManager);
			close(fd);
			userManager.removeUser(fd);
			fds.erase(fds.begin() + i);
			i--;
		}
	}

	if (wBuf.empty() && info.status == -1)
	{
		std::cout << "Closing connection for FD " << fd << " (QUIT processed) " << std::endl;
		close(fd);
		userManager.removeUser(fd);
		fds.erase(fds.begin() + i);
		i--;
		return ;
	}
}

void handleServerPollIn(Server &irc, User &userManager)
{
    try
    {
        std::pair<int, std::string> client = irc.acceptClient();
        userManager.addUserMapInt(client.first);
        userManager.setHostName(client.first, client.second);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
}


void startServerLoop(Server &irc)
{
    User userManager;
    Channel channelManager;

    bool stop = false;

    struct pollfd stdin_pfd;
    stdin_pfd.fd = STDIN_FILENO;
    stdin_pfd.events = POLLIN;
    stdin_pfd.revents = 0;
    irc.getFds().push_back(stdin_pfd);

    while (!stop)
    {
        std::vector<struct pollfd> &fds = irc.getFds();

        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].fd == STDIN_FILENO || fds[i].fd == irc.getSocket())
                continue;
            if (!userManager.getUserInfo(fds[i].fd).writeBuffer.empty() ||
                userManager.getUserInfo(fds[i].fd).status == -1)
                fds[i].events |= POLLOUT;
        }

        int poll_ret = poll(&fds[0], fds.size(), -1);

        if (poll_ret == -1)
            break;

        for (size_t i = 0; i < fds.size(); ++i)
        {
            if (fds[i].fd == STDIN_FILENO)
            {
                if (fds[i].revents & POLLIN)
                {
                    std::string input;
                    std::getline(std::cin, input);
                    if (input == "quit" || input == "exit")
                        stop = true;
                }
                continue;
            }

            if (fds[i].fd == irc.getSocket())
            {
                if (fds[i].revents & POLLIN)
                    handleServerPollIn(irc, userManager);
                continue;
            }

            if (fds[i].revents & (POLLERR | POLLHUP))
            {
                std::cout << "Client " << fds[i].fd << " error/hangup." << std::endl;
                cleanupUserChannels(fds[i].fd, userManager, channelManager);
                close(fds[i].fd);
                userManager.removeUser(fds[i].fd);
                fds.erase(fds.begin() + i);
                i--;
                continue;
            }

            if (fds[i].revents & POLLIN)
            {
                if (handleClientPollIn(fds[i].fd, userManager, channelManager, fds, i, irc.getPass()))
                    continue;
            }

            if (!userManager.getUserInfo(fds[i].fd).writeBuffer.empty() ||
						userManager.getUserInfo(fds[i].fd).status == -1)
                fds[i].events |= POLLOUT;

            if (fds[i].revents & POLLOUT)
            {
                handleClientPollOut(irc, userManager, channelManager, i);
            }
        }
    }

    std::vector<struct pollfd> &fds = irc.getFds();
    std::string shutdownMsg = "ERROR :Server shutting down\r\n";
    for (size_t i = 0; i < fds.size(); ++i)
    {
        if (fds[i].fd != irc.getSocket() && fds[i].fd != STDIN_FILENO)
            send(fds[i].fd, shutdownMsg.c_str(), shutdownMsg.size(), 0);
    }
}
