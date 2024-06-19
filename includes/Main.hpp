#ifndef MAIN_HPP
#define MAIN_HPP

#include <iostream>
#include <string>
#include <sys/event.h>
#include <arpa/inet.h>
#include <sys/event.h>
#include <fcntl.h>
#include <err.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <sstream>

#define BUF_SIZE 1024

// PASS
#define ERR_NEEDMOREPARAMS "Not enough parameters\r\n"
#define ERR_PASSWDMISMATCH "Password incorrect\r\n"
#define ERR_ALREADYREGISTRED "You may not reregister\r\n"

// NICK
#define ERR_NICKNAMEINUSE "Nickname is already in use\r\n"
#define ERR_NICKCOLLISION "Nickname collision KILL from\r\n"
#define ERR_NONICKNAMEGIVEN "No nickname given\r\n"
#define ERR_ERRONEUSNICKNAME "Erroneus nickname\r\n"

// PRIVMSG
#define ERR_NOSUCHCHANNEL "No such channel\r\n"
#define ERR_NOSUCHNICK "No such nick/channel\r\n"

// PART
#define ERR_NOSUCHCHANNEL "No such channel\r\n"
#define ERR_NOTONCHANNEL "You're not on that channel\r\n"

// KICK
#define ERR_CHANOPRIVSNEEDED "You're not channel operator\r\n"
#define ERR_USERNOTINCHANNEL "They aren't on that channel\r\n"

// COMMON
#define ERR_NOTREGISTERED "You have not registered\r\n"

// MODE
#define ERR_UMODEUNKNOWNFLAG "Unknown MODE flag\r\n"
#define ERR_USERSDONTMATCH "Can't change mode for other users\r\n"
#define ERR_UNKNOWNMODE "is unknown mode char to me\r\n"
#define ERR_CHANNELISFULL "Cannot join channel (+l)\r\n"

// JOIN
#define ERR_INVITEONLYCHAN "Cannot join channel (+i)\r\n"
#define ERR_BADCHANNELKEY "Cannot join channel (+k)\r\n"

// TOPIC
#define RPL_NOTOPIC "No topic is set\r\n"

// INVITE
#define ERR_USERONCHANNEL "is already on channel\r\n"
#define RPL_INVITING "Inviting\r\n"

#endif