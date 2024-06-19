#include "../includes/Command.hpp"

void Command::mode(int fd, std::vector<std::string> cmdVector)
{	// "/MODE <channel>	 <+/- i, t, k, l, o>   <t,k,l,o 에 해당하는 values 순서지켜서!!>"
	
	// ** 채널모드 '조회' 출력 결과 <-> 채널모드 '설정' 출력 결과 = 다름!!! **
	// 채널 조회란? 채널 모드를 조회하려면 단순히 MODE 명령어 뒤에 채널 이름만 입력
		// MODE #channel
		// 324 #example +kt secret
	// 채널 설정이란? 채널 모드를 설정할 때는 MODE 명령어 뒤에 채널 이름과 설정하고자 하는 모드를 지정
		// MODE #channel +o nickname
		// :nickname!user@host MODE #channel +o othernickname
	
	// 설정은, 채널 조회처럼 "324  "<channel> <mode> <mode params>" '324' 응답코드 없음!

	Client &client = _server.getClientList().find(fd)->second;
	if (cmdVector.size() < 2)
	{
		ERROR_needmoreparams_461(client);
		return;
	}
	std::string argvChannelName = cmdVector[1];	// #channel of /Mode 
	Channel *channel = _server.findChannel(argvChannelName);
	// findChannel 반환값 : (Channel*) or (NULL)
	
	// 에러
	if ((channel == NULL) && (client.getNickname() != argvChannelName))
	{	// /Mode 인자인 '#channel'을 서버에 저장된 채널 리스트에서 못 찾았고
		// and 애초에 (fd != argvChannelName) 일 때
		ERROR_nosuchchannel_403(client, argvChannelName);	// (Client &, std::string)
		return;
	}

	// 채널 조회	ex) MODE #channel
	if ((channel != NULL) && (cmdVector.size() == 2))
	{	// /Mode 인자인 '#channel'을 서버에 저장된 채널 리스트에서 찾았고!
		// and, "/Mode #channel" 입력됐을 때
        std::string modeParams = "";
		if (channel->getMode().find('t') != std::string::npos)
			modeParams += " " + channel->getTopic();
        if (channel->getMode().find('k') != std::string::npos)
            modeParams += " " + channel->getKey();
        if (channel->getMode().find('l') != std::string::npos)
		{
			std::ostringstream oss;	// ostringstream은 output stream의 약자로, 문자열을 출력하기 위한 스트림
            oss << channel->getLimit();
            modeParams += " " + oss.str();
		}			
		client.appendReciveBuf("324" + argvChannelName + " +" + channel->getMode() + modeParams + "\r\n");
        return;
		// == "<channel> <mode> <mode params>"
		// == ex) 324 #channel +kt secret
		// <mode params> == <+/- i, t, k, l, o> mode 뒤에 오는 "<t,k,l 에 해당하는 values>"
	}

	if (channel != NULL && !channel->diffOperator(fd))
	{	// 채널이 존재하고, 채널 오퍼레이터가 아닐 때
		ERROR_chanoprivsneeded_482(client, argvChannelName);	// (Client &, std::string)
		return;
	}

	std::string modeArgv = cmdVector[2];	// ex) <+/- i, t, k, l, o>
	std::string message = "";
    std::vector<std::string> modeValueList;	// <t,k,l,o 에 해당하는 values>
    unsigned int modeValueCnt = 3;
	// "/Mode #channel +i <modeValueList>" 이므로 Value는 4부터 시작이므로, 3으로 초기화 해서, Value 만나면 그때서야 확신 갖고 ++증가
	char sign = '\0';

	size_t idx = 0;
	while (idx < modeArgv.length() && (modeArgv[idx] == '+' || modeArgv[idx] == '-'))
	{
		sign = modeArgv[idx];
		idx++;
	}
	if (modeArgv.length() == 1)				// +혹은 -만 입력됐을 때 종료
		return;

	// 채널모드 설정 ex) "+o nickname"
	for (size_t i = idx; i < modeArgv.length(); i++)
    {
        if (modeArgv[i] == '+' || modeArgv[i] == '-')
        {
            sign = modeArgv[i];
            continue;
        }

        bool isSetMode = false;				// 모드 '정상' 설정됐는지 여부
        if (modeArgv[i] == 'i')
        {
            if (sign == '+' && channel->diffMode('i'))	// +들어왔는데 이미 채널에 i 모드가 설정되어 있을 때
                continue;								// 이미 설정되어 있다면 무시하고 넘어감
            if (sign == '-' && !channel->diffMode('i'))	// -들어왔는데 채널에 i 모드가 설정되어 있지 않을 때
                continue;
            channel->setMode('i', sign, fd);
            isSetMode = true;
        }
        else if (modeArgv[i] == 't')	// 설정 허용만, /TOPIC 해야 비로소 토픽 설정됨
        {
            if (sign == '+' && channel->diffMode('t'))
                continue;
            if (sign == '-' && !channel->diffMode('t'))
                continue;
            channel->setMode('t', sign, fd);
            isSetMode = true;
        }
        else if (modeArgv[i] == 'k')
        {
            if (sign == '-' && !channel->diffMode('k'))
                continue;
            if (cmdVector.size() > modeValueCnt)
            {
                channel->setMode('k', sign, fd);
                if (sign == '+')
                {
                    channel->setKey(cmdVector[modeValueCnt]);
                }
                else if (sign == '-')
                {
                    channel->setKey("");
                }
                isSetMode = true;
                modeValueList.push_back(cmdVector[modeValueCnt]);	// <t,k,l,o 에 해당하는 values> 저장
                modeValueCnt++;
            }
        }
        else if (modeArgv[i] == 'l')
        {
            if (sign == '-' && !channel->diffMode('l'))	//해제 부호인데, 설정 안되어 있으면 해제ㄴㄴ 무시
                continue;
            if (cmdVector.size() > modeValueCnt)
            {
                std::string limitValue = cmdVector[modeValueCnt].c_str();	//  <t,k,l,o 에 해당하는 values> 중 ValueCnt ++증가된 상태
                bool isDigit = true;
                for (size_t j = 0; j < limitValue.length(); ++j)
                {
                    if (!isdigit(limitValue[j]))
                    {
                        isDigit = false;
                        break;
                    }
                }
                if (isDigit == false)
                {
                    modeValueCnt++;
                    continue;
                }
                int limit = atoi(limitValue.c_str());
                if (limit < 0)
                {
                    modeValueCnt++;
                    continue;
                }
                channel->setMode('l', sign, fd);
                if (sign == '+')
                {
                    channel->setLimit(limit);
                }
                isSetMode = true;
                modeValueList.push_back(cmdVector[modeValueCnt]);
                modeValueCnt++;
            }
        }
        else if (modeArgv[i] == 'o')
        {
            if (cmdVector.size() <= modeValueCnt)	
            {
				continue;				// /Mode #channel +o 입력했는데, nickname이 없을 때 +o 무시
            }
            std::map<int, Client>::iterator target = _server.findClient(cmdVector[modeValueCnt]);	// +o 뒤에 오는 "nickname" 찾는다
            if (target == _server.getClientList().end())	// nickname 못 찾았을 때 에러
            {
				ERROR_nosuchnick_401(client, cmdVector[modeValueCnt]);
                return;
            }
            else	// nickname 찾았을 때
            {
                if (client.getNickname() == target->second.getNickname())	// 자기 자신에게는 +o 불가
                {
                    return;
                }
				if (!(channel->diffClientInChannel(target->second.getClientFd())))	// 채널에 없는 클라이언트에게 +o 불가
                {
					ERROR_usernotinchannel_441(client, cmdVector[modeValueCnt], cmdVector[1]);
                    return;
                }
                else if (sign == '+')
                {
                    channel->addOperatorFd(target->second.getClientFd());
                    isSetMode = true;
                    modeValueList.push_back(cmdVector[modeValueCnt]);
                    modeValueCnt++;
                }
                else if (sign == '-')
                {
                    channel->removeOperatorFd(target->second.getClientFd());
                    isSetMode = true;
                    modeValueList.push_back(cmdVector[modeValueCnt]);
                    modeValueCnt++;
                }
            }
        }
        else		// <+/- i, t, k, l, o> 중에 없으면 에러
        {
			ERROR_unknownmode_472(client, modeArgv[i]);
            continue;
        }
        if (isSetMode)	// 모드 '정상' 설정하고 끝났다면
        {
            if (message.empty())
                message += sign;	// ex) '+' 차곡차곡
            message += modeArgv[i];	// ex) +'itkl' 차곡차곡
        }
    }


	// 
    for (size_t i = 0; i < modeValueList.size(); ++i)
    {
        message += " " + modeValueList[i];	// ex) "+l -k 2 key"
    }
    if (message.empty())
        return;
	messageAllChannel(fd, cmdVector[1], "MODE", message);	// #채널에 MODE와 메세지 전달

}