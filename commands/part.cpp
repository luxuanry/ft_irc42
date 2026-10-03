/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   part.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rxue <rxue@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:45:35 by rxue              #+#    #+#             */
/*   Updated: 2026/03/24 14:45:56 by rxue             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../commands.hpp"

void part(User &user, Channel &channels, std::vector<std::string> cmd, int fd)
{
	struct userInfo &info = user.getUserInfo(fd);
	std::string nick = user.getNickName(fd);
	if (nick.empty())
		nick = "*";

	if (info.status < 1)
	{
		info.writeBuffer += ":server 451 " + nick + " :You have not registered\r\n";
		return;
	}

	if (cmd.size() < 2)
	{
		info.writeBuffer += ":server 461 " + nick + " PART :Not enough parameters\r\n";
		return;
	}

	std::string channelName = cmd[1];

	if (!channels.isExist(channelName))
	{
		info.writeBuffer += ":server 403 " + nick + " " + channelName + " :No such channel\r\n";
		return;
	}

	if (info.channelList.find(channelName) == info.channelList.end())
	{
		info.writeBuffer += ":server 442 " + nick + " " + channelName + " :You're not on that channel\r\n";
		return;
	}

	std::string reason = (cmd.size() > 2) ? cmd[2] : "";
	std::string msg = ":" + info.nickName + "!" + info.loginName + "@" + info.hostName + " PART " + channelName;
	if (!reason.empty()) msg += " " + reason;
	msg += "\r\n";

	std::set<int> &channelUsers = channels.getUsers(channelName);
	for (std::set<int>::iterator it = channelUsers.begin(); it != channelUsers.end(); ++it)
		user.setWrtieBuffer(*it, msg);

	channels.removeUserFromChannel(channelName, fd);
	info.channelList.erase(channelName);

	std::cout << "User " << info.nickName << " has left channel " << channelName << std::endl;
}
