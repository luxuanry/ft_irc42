/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pass.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rxue <rxue@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/24 14:45:59 by rxue              #+#    #+#             */
/*   Updated: 2026/03/24 14:46:10 by rxue             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../commands.hpp"

void pass(User &user, std::vector<std::string> cmd, int fd, const std::string serverPassword)
{
    struct userInfo &info = user.getUserInfo(fd);

    std::string nick = user.getNickName(fd);
    if (nick.empty())
        nick = "*";

    if (info.status >= 1)
    {
        info.writeBuffer += ":server 462 " + nick + " :Unauthorized command (already registered)\r\n";
        return;
    }

    if (cmd.size() < 2)
    {
        info.writeBuffer += ":server 461 " + nick + " PASS :Not enough parameters\r\n";
        return;
    }

    if (cmd[1] != serverPassword)
    {
        info.writeBuffer += ":server 464 " + nick + " :Password incorrect\r\n";
        return;
    }

    info.password = cmd[1];
    std::cout << "FD " << fd << " authenticated with correct password." << std::endl;
}