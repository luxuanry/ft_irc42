/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   topic.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lcao <lcao@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/18 14:44:29 by lcao              #+#    #+#             */
/*   Updated: 2026/03/24 14:38:49 by lcao             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../Channel.hpp"
#include "../commands.hpp"
#include <iostream>

void topicCmd(Channel &chanObj, User &userObj, const std::vector<std::string> &cmds, int fd)
{
    std::string nick = userObj.getNickName(fd);

    if (cmds.size() < 2) {
        std::string errMsg = ":server 461 " + nick + " TOPIC :Not enough parameters\r\n";
        userObj.setWrtieBuffer(fd, errMsg);
        return;
    }

    std::string chanName = cmds[1];


    if (!chanObj.isExist(chanName)) {
        std::string errMsg = ":server 403 " + nick + " " + chanName + " :No such channel\r\n";
        userObj.setWrtieBuffer(fd, errMsg);
        return;
    }

    if (!chanObj.isUserInChannel(chanName, fd)) {
        std::string errMsg = ":server 442 " + nick + " " + chanName + " :You're not on that channel\r\n";
        userObj.setWrtieBuffer(fd, errMsg);
        return;
    }

    struct channelInfo &info = chanObj.getChannelInfo(chanName);

   
    if (cmds.size() == 2) {
        if (info.topic.empty()) {
            std::string msg = ":server 331 " + nick + " " + chanName + " :No topic is set\r\n";
            userObj.setWrtieBuffer(fd, msg);
        } else {
            std::string msg = ":server 332 " + nick + " " + chanName + " " + info.topic + "\r\n";
            userObj.setWrtieBuffer(fd, msg);
        }
        return;
    }
    
    else {
        if (info.topicRestriced && !info.operators.count(fd)) {
            std::string errMsg = ":server 482 " + nick + " " + chanName + " :You're not channel operator\r\n";
            userObj.setWrtieBuffer(fd, errMsg);
            return;
        }
        
        std::string fullTopic = "";
        for (size_t i = 2; i < cmds.size(); ++i) {
            fullTopic += cmds[i];
            if (i != cmds.size() - 1) {
                fullTopic += " ";
            }
        }

        info.topic = fullTopic;

        std::string prefix = ":" + nick + "!" + userObj.getLoginName(fd) + "@" + userObj.getHostName(fd);
        std::string broadcastMsg = prefix + " TOPIC " + chanName + " " + info.topic + "\r\n";
        
        for (std::set<int>::iterator it = info.users.begin(); it != info.users.end(); ++it) {
            userObj.setWrtieBuffer(*it, broadcastMsg);
        }
    }
}