#include "../includes/Server.hpp"
#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"
#include "../includes/Command.hpp"

// Server::Server(char *portNum, char *password)
Server::Server(int portNum, std::string password)
{
	_command = new Command(*this);	// Command 객체 생성 -> 왜? 전방선언 하...
	_portNum = portNum;
	_password = password;
	_clientAddrSize = sizeof(_clientAddr);
}

void Server::run()
{
	setServerSock();
	setServerAddr();
	setServerBind();
	setServerListen();
	execute();
}

void Server::setServerSock()
{
	int opiton = 1;
	_serverSock = socket(PF_INET, SOCK_STREAM, 0);
	if (_serverSock == -1)
		throw std::runtime_error("socket error");
	if (setsockopt(_serverSock, SOL_SOCKET, SO_REUSEADDR, &opiton, sizeof(opiton)) == -1)
	{
		close(_serverSock);
		throw std::runtime_error("setsockopt error");
	}
}

void Server::setServerAddr()
{
	memset(&_serverAddr, 0, sizeof(_serverAddr));
	_serverAddr.sin_family = AF_INET;
	_serverAddr.sin_addr.s_addr = INADDR_ANY;
	_serverAddr.sin_port = htons(_portNum);
}

void Server::setServerBind()
{
	if (bind(_serverSock, (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) == -1)
	{
		close(_serverSock);
		throw std::runtime_error("bind error");
	}
}

void Server::setServerListen()
{
	if (listen(_serverSock, MAX_CONNECTION) == -1)
	{
		close(_serverSock);
		throw std::runtime_error("listen error");
	}
}

void Server::kqueueInit()
{
	_kq = kqueue();
	if (_kq == -1)
	{
		close(_serverSock);
		throw std::runtime_error("kqueue error");
	}

	changeEvent(_serverSock, READ, NULL);
    // struct kevent change_event;
    // EV_SET(&change_event, _serverSock, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL); // kqueue 설정
    // _changeList.push_back(change_event);

    if (kevent(_kq, &_changeList[0], _changeList.size(), 0, 0, NULL) == -1) 
        throw std::logic_error("ERROR :: kevent() error");
}

// command <option>

void Server::execute()
{
	kqueueInit();			// kq에 serverSock(리스닝소켓) 등록
	while (1)
	{
		_eventCnt = kevent(_kq, &_changeList[0], _changeList.size(), _eventList, 256, NULL);
		// Kq를 통해 생성된 이벤트 큐(kevent)를 식별할 것이고,
		// changeList에 있는 이벤트들을 _changeList.size()만큼 감시하고,
		// 실제로 이벤트가 발생한 것이 있으면, eventList[256]에 이벤트들을 저장한다 ->
		// 저장하고, 그 수만큼을 반환
		if (_eventCnt == -1)
		{
			close(_serverSock);
			throw std::runtime_error("kevent error");
		}
		_changeList.clear();
		for (int i = 0; i < _eventCnt; i++)
		{
			_curr_event = &_eventList[i];
			if (_curr_event->flags & EV_ERROR)
			{
				if (_curr_event->ident == static_cast<uintptr_t>(_serverSock))
				{
					// closeClient();
					close(_serverSock);
					throw std::runtime_error("server socket error");
				}
				else
				{
					std::cerr << "client socket error" << std::endl;
					// disconnectClient(_curr_event.ident);
				}
			}
			else if (_curr_event->filter == EVFILT_READ)
			{
				if (_curr_event->ident == static_cast<uintptr_t>(_serverSock))
				{
					int clientSock;
					if ((clientSock = accept(_serverSock, NULL, NULL)) == -1)
						throw acceptError();
					std::cout << "accept new client: " << clientSock << std::endl;
					fcntl(clientSock, F_SETFL, O_NONBLOCK);

					changeEvent(clientSock, READ, NULL);
					_clientList.insert(std::make_pair(clientSock, Client(clientSock)));
				}
				else if (_clientList.find(_curr_event->ident) != _clientList.end())
				{
					char buf[1024];
					int n = recv(_curr_event->ident, buf, sizeof(buf), 0);

					if (n <= 0)
					{
						if (n < 0)
							std::cerr << "client read error!" << std::endl;
						// disconnectClient(_curr_event.ident);
					}
					else
					{
						buf[n] = '\0';
						_clientList[_curr_event->ident].appendReciveBuf(buf);
						std::cout << "received data from " << _curr_event->ident << ": " << _clientList[_curr_event->ident].getReciveBuf() << std::endl;
						_command->run(_curr_event->ident);
					}
				}
			}
		}
	}
}

void Server::changeEvent(int ident, int flag, void *udata)
{
	struct kevent temp_event;
	if (flag == READ)
		EV_SET(&temp_event, ident, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, udata);
	else if (flag == WRITE)
		EV_SET(&temp_event, ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, udata);
	_changeList.push_back(temp_event);
}


// 태현 추가
Server::~Server()
{
	std::map<int, Client>::iterator iter;
	iter = _clientList.begin();
	while (iter != _clientList.end())
	{
		close(iter->first);
		iter++;
	}

	std::map<std::string, Channel*>::iterator channelIter;
	channelIter = _channelList.begin();
	while (channelIter != _channelList.end())
	{
		delete (channelIter->second);
		// delete 하는 이유
		// Server::appendNewChannel(int fd, std::string& channelName) 에서
 		// ( ... , 'new Channel'(channelName, fd)));
		// new로 동적할당했기 때문에 Channel 은 꼭 delete 할것!!
		channelIter++;
	}

	_clientList.clear();
	_channelList.clear();
	// clear()는 map의 모든 요소를 제거함
	delete _command;	// new Command(*this); 생성자에서 동적할당했었음
	close(_serverSock);
}

// 태현 추가
std::map<int, Client>::iterator Server::findClient(std::string nickname)
{
	std::map<int, Client>::iterator iter;
	
	iter = _clientList.begin();
	while (iter != _clientList.end())
	{
		if (iter->second.getNickname() == nickname)
			return (iter);
		iter++;
	}
	return (iter);
}
// 태현 추가
std::map<int, Client> &Server::getClientList()
{
	return (_clientList);
}

std::map<std::string, Channel*> &Server::getChannelList()
{
	return (_channelList);
}

// Channel Server::findChannel(std::string channel_name)
// {
// 	std::map<std::string, Channel>::iterator iter;
	
// 	iter = _channelList.find(channel_name);
// 	if (iter != _channelList.end())
// 		return (iter->second);
// 	return (Channel());
// }

Channel* Server::findChannel(std::string channel_name)
{
	std::map<std::string, Channel*>::iterator iter;
	
	iter = _channelList.find(channel_name);
	if (iter != _channelList.end())
		return ((iter->second));
	return (NULL);
}

// 태현 추가
void Server::appendNewChannel(int fd, std::string& channelName)
{
	_channelList.insert(std::make_pair(channelName, new Channel(channelName, fd)));
}

std::string Server::getMessage(int clientSock)
{
	std::string message;
	char buf[1024];
	int n = recv(clientSock, buf, sizeof(buf), 0);
	if (n <= 0)
	{
		if (n < 0)
			std::cerr << "client read error!" << std::endl;
		// disconnectClient(clientSock);
	}
	else
	{
		buf[n] = '\0';
		message = buf;
	}
	return message;
}

std::string Server::getPassword()
{
	return (_password);
}

void Server::removeChannel(std::string channelName)
{
	_channelList.erase(channelName);
}