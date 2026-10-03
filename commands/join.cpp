/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   join.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: suna <suna@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:52:41 by suna              #+#    #+#             */
/*   Updated: 2026/03/24 14:53:07 by suna             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../commands.hpp"

static std::string makePrefix(User &user, int fd)
{
	return ":" + user.getNickName(fd) + "!" + user.getLoginName(fd) + "@" + user.getHostName(fd);
}

void join(User &user, Channel &channel, std::vector<std::string> cmd, int fd)
{
	struct userInfo &info = user.getUserInfo(fd);

	if (cmd.size() < 2)
	{
		info.writeBuffer += ":server 461 " + user.getNickName(fd) + " JOIN :Not enough parameters\r\n";
		return;
	}

	std::string channelName = cmd[1];

	if (channelName.empty() || channelName[0] != '#')
	{
		info.writeBuffer += ":server 403 " + user.getNickName(fd) + " " + channelName + " :No such channel\r\n";
		return;
	}

	if (channel.isUserInChannel(channelName, fd))
		return;

	if (!channel.isExist(channelName))
	{
		channel.addChannel(channelName);
		channel.addOperator(channelName, fd);
	}
	else
	{
		channelInfo &chInfo = channel.getChannelInfo(channelName);

		if (chInfo.inviteOnly && !channel.isInvited(channelName, fd))
		{
			info.writeBuffer += ":server 473 " + user.getNickName(fd) + " " + channelName + " :Cannot join channel (+i)\r\n";
			return;
		}

		if (chInfo.hasLimit && (int)chInfo.users.size() >= chInfo.limit)
		{
			info.writeBuffer += ":server 471 " + user.getNickName(fd) + " " + channelName + " :Cannot join channel (+l)\r\n";
			return;
		}

		if (chInfo.hasKey)
		{
			std::string providedKey = cmd.size() > 2 ? cmd[2] : "";
			if (providedKey != chInfo.key)
			{
				info.writeBuffer += ":server 475 " + user.getNickName(fd) + " " + channelName + " :Cannot join channel (+k)\r\n";
				return;
			}
		}
	}

	channel.addUserToChannel(channelName, fd);
	info.channelList.insert(channelName);

	channel.getChannelInfo(channelName).inviteList.erase(fd);

	std::string prefix = makePrefix(user, fd);
	std::string joinMsg = prefix + " JOIN " + channelName + "\r\n";

	std::set<int> &users = channel.getUsers(channelName);
	std::set<int>::iterator it;
	for (it = users.begin(); it != users.end(); ++it)
		user.setWrtieBuffer(*it, joinMsg);

	channelInfo &chInfo = channel.getChannelInfo(channelName);
	if (!chInfo.topic.empty())
		info.writeBuffer += ":server 332 " + user.getNickName(fd) + " " + channelName + " TOPIC " + chInfo.topic + "\r\n";

	std::string nameList;
	for (it = users.begin(); it != users.end(); ++it)
	{
		if (!nameList.empty())
			nameList += " ";
		if (channel.isOperator(channelName, *it))
			nameList += "@";
		nameList += user.getNickName(*it);
	}
	info.writeBuffer += ":server 353 " + user.getNickName(fd) + " = " + channelName + " :" + nameList + "\r\n";

	info.writeBuffer += ":server 366 " + user.getNickName(fd) + " " + channelName + " :End of /NAMES list\r\n";
}
