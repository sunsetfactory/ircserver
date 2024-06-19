#include "../includes/Command.hpp"

void Command::privmsg(int fd, std::vector<std::string> cmdVector)
{
	Client &client = _server.getClientList().find(fd)->second;
	if (cmdVector.size() < 3) // 명령어에 필요한 인자가 부족한 경우 :
	{
		// ERR_NEEDMOREPARAMS = "Not enough parameters\r\n"
		ERROR_needmoreparams_461(client);
		return;
	}
	std::vector<std::string> msgArgv1 = split(cmdVector[1], ',');
	std::vector<std::string>::iterator msgArgv1Iter = msgArgv1.begin();
	for (; msgArgv1Iter != msgArgv1.end(); msgArgv1Iter++) // 채널 목록을 순회
	{
		if ((*msgArgv1Iter)[0] == '#' || (*msgArgv1Iter)[0] == '&') // 채널인 경우 :
		{
			// msgToChannel(msgArgv1Iter, cmdVector, client);
			messageAllChannel(fd, *msgArgv1Iter, "PRIVMSG", channelMessage(2, cmdVector));
		}
		else
		{
			// 클라이언트가 존재하는지 확인
			std::map<int, Client>::iterator client = _server.findClient(*msgArgv1Iter);
			// 클라이언트가 존재하는 경우
			if (client != _server.getClientList().end())
			{
				// 메시지 생성
				std::string message = channelMessage(2, cmdVector);
				// 클라이언트에게 메시지 버퍼에 추가
				client->second.appendReciveBuf(":" + client->second.getNickname() + " PRIVMSG " + client->second.getNickname() + " :" + message + "\r\n");
			}
			// 클라이언트가 존재하지 않는 경우
			else
			{
				// ERROR_nosuchnick_401(*msgArgv1Iter);
				// ERR_NOSUCHNICK = "No such nick/channel\r\n"
				ERROR_nosuchnick_401(client->second, *msgArgv1Iter);
				continue;
			}
		}
	}
}

// void Command::msgToChannel(std::vector<std::string>::iterator msgArgv1Iter, std::vector<std::string> cmdVector, Client &client)
// {
// 	Channel *channel = _server.findChannel(*msgArgv1Iter);
// 	if (channel) // 채널이 존재하는 경우 :
// 	{
// 		std::string message = channelMessage(2, cmdVector); // 메시지 생성
// 		// 채널에 메시지 전송
// 		channelPRIVMSG(message, client, channel);
// 	}
// 	else // 채널이 존재하지 않는 경우 :
// 	{
// 		// ERR_NOSUCHCHANNEL = "No such channel\r\n"
// 		ERROR_nosuchchannel_403(client, *msgArgv1Iter);
// 	}
// }