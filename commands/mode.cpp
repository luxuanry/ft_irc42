/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mode.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lcao <lcao@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/18 14:42:30 by lcao              #+#    #+#             */
/*   Updated: 2026/03/24 14:42:46 by lcao             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../Channel.hpp"
#include "../commands.hpp"
#include <iostream>
#include <cstdlib>

void modeCmd(Channel &chanObj, User &userObj, const std::vector<std::string> &cmds, int fd)
{
    std::string execNick = userObj.getNickName(fd);

    if (cmds.size() < 2) {
        std::string errMsg = ":server 461 " + execNick + " MODE :Not enough parameters\r\n";
        userObj.setWrtieBuffer(fd, errMsg);
        return;
    }

    std::string chanName = cmds[1];

    if(!chanObj.isExist(chanName)){
        std::string errMsg = ":server 403 " + execNick + " " + chanName + " :No such channel\r\n";
        userObj.setWrtieBuffer(fd, errMsg);
        return;
    }

    struct channelInfo &info = chanObj.getChannelInfo(chanName);

    if (cmds.size() == 2) {
        std::string activeModes = "+";
        if (info.inviteOnly) activeModes += "i";
        if (info.topicRestriced) activeModes += "t";
        if (info.hasKey) activeModes += "k";
        if (info.hasLimit) activeModes += "l";
        
        std::string reply = ":server 324 " + execNick + " " + chanName + " " + activeModes + "\r\n";
        userObj.setWrtieBuffer(fd, reply);
        return;
    }

    if (!info.operators.count(fd)) {
        std::string errMsg = ":server 482 " + execNick + " " + chanName + " :You're not channel operator\r\n";
        userObj.setWrtieBuffer(fd, errMsg);
        return;
    }


    std::string modeString = cmds[2];
    bool isAdding = true;
    size_t argIndex = 3;
    
    std::string modeChanges = ""; 
    std::string modeArgs = "";

    for (size_t i = 0; i < modeString.length(); ++i) {
        char c = modeString[i];

        if (c == '+' && i == 0) {
            isAdding = true;
            modeChanges += "+";
        } else if (c == '-' && i == 0) {
            isAdding = false;
            modeChanges += "-";
        } else {
            switch (c) {
                case 'i':
                    info.inviteOnly = isAdding;
                    modeChanges += "i";
                    break;
                case 't':
                    info.topicRestriced = isAdding;
                    modeChanges += "t";
                    break;
                case 'k':
                    if (isAdding) {
                        if (argIndex < cmds.size()) {
                            info.hasKey = true;
                            info.key = cmds[argIndex];
                            modeChanges += "k";
                            modeArgs += " " + cmds[argIndex];
                            argIndex++;
                        }
                    } else {
                        info.hasKey = false;
                        info.key = "";
                        modeChanges += "k";
                    }
                    break;
                case 'l':
                    if (isAdding) {
                        if (argIndex < cmds.size()) {
                            info.hasLimit = true;
                            info.limit = std::atoi(cmds[argIndex].c_str());
                            modeChanges += "l";
                            modeArgs += " " + cmds[argIndex];
                            argIndex++;
                        }
                    } else {
                        info.hasLimit = false;
                        modeChanges += "l";
                    }
                    break;
                case 'o':
                    if (argIndex < cmds.size()) {
                        std::string targetUser = cmds[argIndex];
                        int targetFd = userObj.getFdByNick(targetUser);
                        
                        if (targetFd != -1) {
                            if (isAdding)
                                info.operators.insert(targetFd);
                            else
                                info.operators.erase(targetFd);
                            
                            modeChanges += "o";
                            modeArgs += " " + targetUser;
                        } else {
                            std::string reply = ":server 401 " + execNick + " " + chanName + " " + "\r\n";
                            userObj.setWrtieBuffer(fd, reply);
                        }
                        argIndex++;
                    }
                    break;
                default:
                    std::string errMsg = ":server 472 " + execNick + " " + c + " :is unknown mode char to me\r\n";
                    userObj.setWrtieBuffer(fd, errMsg);
                    break;
            }
        }
    }

    if (modeChanges != "+" && modeChanges != "-" && !modeChanges.empty()) {
        std::string prefix = ":" + execNick + "!" + userObj.getLoginName(fd) + "@" + userObj.getHostName(fd);
        std::string broadcastMsg = prefix + " MODE " + chanName + " " + modeChanges + modeArgs + "\r\n";
        
        for (std::set<int>::iterator it = info.users.begin(); it != info.users.end(); ++it) {
            userObj.setWrtieBuffer(*it, broadcastMsg);
        }
    }
}