/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   invite.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: suna <suna@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:52:22 by suna              #+#    #+#             */
/*   Updated: 2026/03/24 14:52:35 by suna             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../commands.hpp"

void invite(User &user, Channel &channel, std::vector<std::string> cmd, int fd)
{
	struct userInfo &info = user.getUserInfo(fd);
	std::string nick = user.getNickName(fd);

	if (cmd.size() < 3)
	{
		info.writeBuffer += ":server 461 " + nick + " INVITE :Not enough parameters\r\n";
		return;
	}

	std::string targetNick = cmd[1];
	std::string channelName = cmd[2];

	if (!channel.isExist(channelName))
	{
		info.writeBuffer += ":server 403 " + nick + " " + channelName + " :No such channel\r\n";
		return;
	}

	if (!channel.isUserInChannel(channelName, fd))
	{
		info.writeBuffer += ":server 442 " + nick + " " + channelName + " :You're not on that channel\r\n";
		return;
	}

	channelInfo &chInfo = channel.getChannelInfo(channelName);
	if (chInfo.inviteOnly && !channel.isOperator(channelName, fd))
	{
		info.writeBuffer += ":server 482 " + nick + " " + channelName + " :You're not channel operator\r\n";
		return;
	}

	if (!user.isExist(targetNick))
	{
		info.writeBuffer += ":server 401 " + nick + " " + targetNick + " :No such nick/channel\r\n";
		return;
	}

	int targetFd = user.getFdByNick(targetNick);

	if (channel.isUserInChannel(channelName, targetFd))
	{
		info.writeBuffer += ":server 443 " + nick + " " + targetNick + " " + channelName + " :is already on channel\r\n";
		return;
	}

	channel.addToInviteList(channelName, targetFd);

	info.writeBuffer += ":server 341 " + nick + " " + targetNick + " " + channelName + "\r\n";

	std::string prefix = ":" + nick + "!" + user.getLoginName(fd) + "@" + user.getHostName(fd);
	user.setWrtieBuffer(targetFd, prefix + " INVITE " + targetNick + " " + channelName + "\r\n");
}
