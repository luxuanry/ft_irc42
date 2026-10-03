/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   privmsg.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: suna <suna@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:53:47 by suna              #+#    #+#             */
/*   Updated: 2026/03/24 14:54:07 by suna             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../commands.hpp"

static std::string extractMessage(std::vector<std::string> &cmd)
{
	if (cmd.size() < 3)
		return "";
	std::string msg = cmd[2];
	for (size_t i = 3; i < cmd.size(); i++)
		msg += " " + cmd[i];
	if (!msg.empty() && msg[0] == ':')
		msg = msg.substr(1);
	return msg;
}

void privmsg(User &user, Channel &channel, std::vector<std::string> cmd, int fd)
{
	struct userInfo &info = user.getUserInfo(fd);
	std::string nick = user.getNickName(fd);

	if (cmd.size() < 2)
	{
		info.writeBuffer += ":server 411 " + nick + " :No recipient given (PRIVMSG)\r\n";
		return;
	}

	if (cmd.size() < 3)
	{
		info.writeBuffer += ":server 412 " + nick + " :No text to send\r\n";
		return;
	}

	std::string target = cmd[1];
	std::string message = extractMessage(cmd);
	std::string prefix = ":" + nick + "!" + user.getLoginName(fd) + "@" + user.getHostName(fd);

	if (!target.empty() && target[0] == '#')
	{
		if (!channel.isExist(target))
		{
			info.writeBuffer += ":server 403 " + nick + " " + target + " :No such channel\r\n";
			return;
		}

		if (!channel.isUserInChannel(target, fd))
		{
			info.writeBuffer += ":server 404 " + nick + " " + target + " :Cannot send to channel\r\n";
			return;
		}

		std::string privMsg = prefix + " PRIVMSG " + target + " :" + message + "\r\n";
		std::set<int> &users = channel.getUsers(target);
		std::set<int>::iterator it;
		for (it = users.begin(); it != users.end(); ++it)
		{
			if (*it != fd)
				user.setWrtieBuffer(*it, privMsg);
		}
	}
	else
	{
		if (!user.isExist(target))
		{
			info.writeBuffer += ":server 401 " + nick + " " + target + " :No such nick/channel\r\n";
			return;
		}

		int targetFd = user.getFdByNick(target);
		std::string privMsg = prefix + " PRIVMSG " + target + " :" + message + "\r\n";
		user.setWrtieBuffer(targetFd, privMsg);
	}
}
