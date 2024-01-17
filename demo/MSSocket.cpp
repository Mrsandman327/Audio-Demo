#include "MSSocket.h"
#include <stdio.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <future>
#include <algorithm>

#ifdef __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h> //close()
#include <netdb.h> //gethostbyname
#include <fcntl.h>

#define INVALID_SOCKET  (unsigned int)(~0)
#define SOCKET_ERROR				  (-1)
#define closesocket close
#define SD_RECEIVE 	SHUT_RD
#define SD_SEND 	SHUT_WD
#define SD_BOTH 	SHUT_RDWR
#define WOULDBLOCK  EWOULDBLOCK

#elif  defined(_WIN32)
#include <winsock2.h>  
#ifdef _MSC_VER
#pragma comment(lib,"ws2_32.lib") 
#endif
#define WOULDBLOCK  WSAEWOULDBLOCK
#define socklen_t int
#endif

#define _Blocking			0	/*堵塞I/O*/
#define _NonBlocking		1   /*非堵塞I/O(非堵塞模式下使用多路IO复用模型，window下使用select,linux下可选择select,epoll)*/
#define _IOMODEL_  _NonBlocking

#define READTHREAD 4
#define MAXREQUEST 10000

CMSSocket::CMSSocket()
: _pthreadpool(nullptr)
{
	init_skt();
	_pthreadpool = new ThreadPool(READTHREAD, MAXREQUEST);
}

CMSSocket::~CMSSocket()
{
	if (nullptr != _pthreadpool)
	{
		delete _pthreadpool;
		_pthreadpool = nullptr;
	}
	uninit_skt();
}

void CMSSocket::attach_observable(CSocketObservable *observer)
{
	_observerlist.push_back(observer);
}

void CMSSocket::dttach_observable(CSocketObservable* observer)
{
	for (auto it = _observerlist.begin(); it != _observerlist.end(); it++)
	{
		if (*it == observer)
		{
			_observerlist.remove(observer);
			break;
		}
	}
}

void CMSSocket::notify_observable(int socket, socketevent event)
{
	for (auto it = _observerlist.begin(); it != _observerlist.end(); it++)
	{
		(*it)->Update(socket, event);
	}
}

void CMSSocket::addclientsock(int s)
{
	std::lock_guard<std::mutex> lock(_mutexclient);
	_clientsocklist.push_back(s);
}

void CMSSocket::delclientsock(int s)
{
	std::lock_guard<std::mutex> lock(_mutexclient);
	int size = static_cast<int>(_clientsocklist.size());
	for (int i = 0; i < size; ++i)
	{
		if (_clientsocklist[i] == s)
		{
			_clientsocklist.erase(_clientsocklist.begin() + i);
			break;
		}
	}
}

int	CMSSocket::getmaxsocket()
{
	int maxfx = -1;
	int size = static_cast<int>(_clientsocklist.size());
	for (int i = 0; i < size; ++i)
	{
		if (_clientsocklist[i] > maxfx)
		{
			maxfx = _clientsocklist[i];
		}
	}

	return maxfx;
}

int CMSSocket::geterror_skt()
{
#ifdef __linux__
	return errno;
#elif  defined(_WIN32)
	return GetLastError();
#endif

}

int CMSSocket::init_skt()
{
#ifdef _WIN32
	unsigned short sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (0 != (WSAStartup(WS_VERSION_CHOICE1, &wsaData)) &&
		(0 != (WSAStartup(WS_VERSION_CHOICE2, &wsaData))))
	{
		printf("socket---WSAStartup error,code:%d\n", geterror_skt());
		return -1;
	}
	if ((wsaData.wVersion != WS_VERSION_CHOICE1) &&
		(wsaData.wVersion != WS_VERSION_CHOICE2))
	{
		printf("socket---WSAStartup error,code:%d\n", geterror_skt());
		WSACleanup();
		return -1;
	}
#endif
	return 0;
}

int CMSSocket::uninit_skt()
{
#ifdef _WIN32
	WSACleanup();
#endif
	return 1;
}

int CMSSocket::make_skt()
{
	int s = static_cast<int>(socket(AF_INET, SOCK_STREAM, IPPROTO_IP));
	if (INVALID_SOCKET == s) {
		printf("socket---socket error,code:%d\n", geterror_skt());
		return SOCKET_ERROR;
	}
	return s;
}

void CMSSocket::close_skt(int s)
{
	if (s) 
	{
		closesocket(s);
		s = 0;
	}
}

void CMSSocket::clientclose(int s)
{
	/* 通知订阅者 */
	this->notify_observable(s, event_clientdiscon);

	/* 关闭发送和接收操作 */
	shutdown(s, SD_BOTH);
	close_skt(s);
	delclientsock(s);
}

void CMSSocket::severclose(int s)
{
	/* 通知订阅者 */
	this->notify_observable(s, event_servercolse);

	shutdown(s, SD_BOTH);
	close_skt(s);

	for (auto it = _clientsocklist.begin(); it != _clientsocklist.end(); it++)
	{
		shutdown(*it, SD_BOTH);
		close_skt(*it);
	}
	_clientsocklist.clear();
}

void CMSSocket::setnoblocking_skt(int s)
{
#ifdef _WIN32
	unsigned long ul = 1;
	ioctlsocket(s, FIONBIO, (unsigned long *)&ul);
#else
	int flags = fcntl(s, F_GETFL, 0);
	fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

void CMSSocket::addepollfd_skt(int epollfd, int fd, bool oneshot)
{
#ifdef __linux__
	epoll_event event;
	event.data.fd = fd;

	/*设置要处理的事件类型(可读事件，边沿触发)*/
	event.events = EPOLLIN | EPOLLET;
	/*采用EPOLLONETSHOT事件*/
	if (oneshot)
	{
		event.events |= EPOLLONESHOT;
	}

	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
#endif
}

void CMSSocket::delepollfd_skt(int epollfd, int fd, bool oneshot)
{
#ifdef __linux__
	epoll_event event;
	event.data.fd = fd;

	/*设置要处理的事件类型(可读事件，边沿触发)*/
	event.events = EPOLLIN | EPOLLET;

	/*采用EPOLLONETSHOT事件*/
	if (oneshot)
	{
		event.events |= EPOLLONESHOT;
	}

	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &event);
#endif
}

void CMSSocket::resetonshot_skt(int epollfd, int fd)
{
#ifdef __linux__
	epoll_event event;
	event.data.fd = fd;

	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
#endif
}

int CMSSocket::send_skt(int s, char *data, int len)
{
	int length;
	if (SOCKET_ERROR == (length = send(s, data, len, 0)))
	{
		printf("socket---send error,code:%d\n", geterror_skt());
		if (geterror_skt() == WOULDBLOCK)
			return 0;
		close_skt(s);
		return -1;
	}
	else if (length != len) {
		close_skt(s);
		return -1;
	}
	return 0;
}

int CMSSocket::receive_skt(int s, char *data, int len)
{
	if (SOCKET_ERROR == (len = recv(s, data, len, 0)))
	{
		printf("socket---recv error,code:%d\n", geterror_skt());
		close_skt(s);
		return len;
	}
	if (len == 0)
	{
		close_skt(s);
	}
	return len;
}

bool CMSSocket::connect_skt(int s, std::string addr, int port)
{
	unsigned long lAddr = inet_addr(addr.c_str());

	if (INADDR_NONE == lAddr)
	{
		hostent *h = gethostbyname(addr.c_str());/*查看是否是域名*/
		if (nullptr == h)
			return false;
		else
			lAddr = *((unsigned long *)(h->h_addr));
	}   

	sockaddr_in sockAddr;
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = lAddr;
	sockAddr.sin_port = htons(port);

	if (connect(s, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		printf("socket---connect error,code:%d\n", geterror_skt());
		close_skt(s);
		return false;
	}
	return true;
}

bool CMSSocket::listen_skt(int s, std::string addr, int port)
{
	unsigned long lAddr = inet_addr(addr.c_str());

	if (addr.empty())
		lAddr = htonl(INADDR_ANY);

	if (INADDR_NONE == lAddr)
	{
		hostent *h = gethostbyname(addr.c_str());
		if (h == nullptr)
			lAddr = htonl(INADDR_ANY);
		else
			lAddr = *((unsigned long *)(h->h_addr));
	}

	/*允许重用本地地址和端口*/
	int breuseaddr = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&breuseaddr, sizeof(int));

	/*设置非堵塞模式*/
#if _IOMODEL_ == _NonBlocking
	setnoblocking_skt(s);
#endif

	sockaddr_in sockAddr;
	//memset(sockAddr.sin_zero, 0, sizeof(struct sockaddr_in));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = lAddr;
	sockAddr.sin_port = htons(port);

	if (bind(s, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		printf("socket---bind error,code:%d\n", geterror_skt());
		return false;
	}

	if (SOCKET_ERROR == listen(s, SOMAXCONN))
	{
		printf("socket---listen error,code:%d\n", geterror_skt());
		return false;
	}

	std::function<void()> serverskt;
#if _IOMODEL_ == _NonBlocking
#ifdef __linux__
	serverskt = std::bind(&CMSSocket::serverepoll_skt, this, s);
#else
	serverskt = std::bind(&CMSSocket::serverselect_skt, this, s);
#endif
#elif _IOMODEL_ == _Blocking
	serverskt = std::bind(&CMSSocket::accpet_skt, this, s);
#endif
	_pthreadpool->append(serverskt);
	return true;
}

void CMSSocket::serverselect_skt(int s)
{
	/*连接的客户端信息*/
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);

	/*保存最大文件描述符*/
	int maxfd = s;

	/*文件描述符*/
	fd_set readfds;

	/*把readfds清空*/
	FD_ZERO(&readfds);

	/*把要监听的sockfd添加到readfds中*/
	FD_SET(s, &readfds);

	while (true)
	{
		/*用select类监听sockfd 阻塞状态*/
		int  ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
		
		if (-1 == ret)
		{
			/*客户端意外关闭,如果正好是maxfd,select就会返回-1，
			需要主动从监听集合中删除maxfd并且重新设置maxfd*/
			FD_CLR(maxfd, &readfds);
			clientclose(maxfd);

			maxfd = s;
			int maxsock = getmaxsocket();
			if (0 < maxsock)
				maxfd = maxsock > s ? maxsock : s;

			printf("socket---select error,code:%d\n", geterror_skt());
			continue;//break;
		}
		else if (0 == ret)
		{
			printf("socket---select timeout\n");
			continue;
		}

		/*判断是否是客户端链接*/
		if (FD_ISSET(s, &readfds))
		{
			/* 接受客户端连接*/
			int clientfd = static_cast<int>(accept(s, (struct sockaddr *)&clientaddr, (socklen_t *)&addrlen));
			if (INVALID_SOCKET != clientfd)
			{
				/*添加到客户socket列表*/
				addclientsock(clientfd);

				/*客户端套接字描述符添加到监听集合中*/
				FD_SET(clientfd, &readfds);
				maxfd = maxfd > clientfd ? maxfd : clientfd;

				/*设置接入的连接为非堵塞*/
				setnoblocking_skt(clientfd);

				/* 通知订阅者 */
				this->notify_observable(clientfd, event_clientaccpet);
				//printf("ip=%s\n", inet_ntoa(clientaddr.sin_addr));

				continue;
			}
			else
			{
				/*服务器关闭*/
				break;
			}
		}
		else
		{
			/*select函数返回时, readfds里只会留下可以读操作的文件描述符，
			其它不可操作的文件描述符会移除掉的。这里需要重新添加*/
			FD_SET(s, &readfds);
			maxfd = maxfd > s ? maxfd : s;
		}

		int clientsize = getclientsocksize();
		for (int i = 0; i < clientsize; i++)
		{
			if (FD_ISSET(_clientsocklist[i], &readfds))
			{
				/* 读取数据*/
				char rbuffer[65535];
				int datalen = receive_skt(_clientsocklist[i], rbuffer, sizeof(rbuffer));
				if (datalen == 0)
				{
					continue;
				}
				else if (SOCKET_ERROR == datalen)
				{
					/*客户端套接字描述符从监听集合中删除*/
					FD_CLR(_clientsocklist[i], &readfds);
					
					/*关闭客户端*/
					clientclose(_clientsocklist[i]);
					--i;--clientsize;

					/*计算描述符最大个数*/
					if (0 < _clientsocklist.size())
						maxfd = *max_element(_clientsocklist.begin(), _clientsocklist.end()) + 1;
					else
						maxfd = -1;
					//break;
				}
				else
				{
					do
					{
						std::lock_guard<std::mutex> lock(_mutexdata);
						recvdata data;
						data.socket = _clientsocklist[i];
						data.size = datalen;
						datalen = datalen > DATAPACKETSIZE ? DATAPACKETSIZE : datalen;
						memcpy(&data.buffer, rbuffer, datalen);
						std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
						_dataqueue.push(recvda);
					} while (0);

					/* 通知订阅者 */
					this->notify_observable(_clientsocklist[i], event_serverrecv);
				}
			}
			else
			{
				/*select函数返回时, readfds里只会留下可以读操作的文件描述符，
				其它不可操作的文件描述符会移除掉的。这里需要重新添加*/
				FD_SET(_clientsocklist[i], &readfds);
				maxfd = maxfd > _clientsocklist[i] ? maxfd : _clientsocklist[i];
			}
		}
	}
}

void CMSSocket::serverepoll_skt(int s)
{
#ifdef __linux__
	/*声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件*/
	struct epoll_event events[10000];

	/*生成用于处理accept的epoll专用的文件描述符*/
	int epfd = epoll_create(256);
	if(-1 == epfd )
	{
		printf("socket---epoll_create error,code:%d\n", geterror_skt());
		return;
	}

	/*注册epoll事件*/
	addepollfd_skt(epfd, s, true);

	while (true)
	{
		/*等待监控事件发生 -1：代表无限等待*/
		int nfds = epoll_wait(epfd, events, 10000, -1);
		if (-1 == nfds)
		{
			if(geterror_skt() != EINTR)
			{
				printf("socket---epoll_wait error,code:%d\n", geterror_skt());
				break;
			}
			else
			{
				/*代码中忽略由于接收调试信号而产生的"错误"返回*/
				continue;
			}
		}
		else if (0 == nfds)
		{
			printf("socket---epoll_wait timeout\n");
			continue;
		}

		for (int i = 0; i < nfds; ++i)
		{
			int sockfd = events[i].data.fd;
			if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				/* 监控到错误或者挂起 */
				printf("epoll error\n");

				clientclose(sockfd);
				continue;
			} 
			else if (sockfd == s) /*有新的连接*/
			{
				/*连接的客户端信息*/
				struct sockaddr_in clientaddr;
				int addrlen = sizeof(clientaddr);
				int clientfd = accept(s, (struct sockaddr *)&clientaddr, (socklen_t *)&addrlen);//accept这个连接
				if (INVALID_SOCKET != clientfd)
				{
					/*添加到客户socket列表*/
					addclientsock(clientfd);

					/*设置接入的连接为非堵塞*/
					setnoblocking_skt(clientfd);

					/* 通知订阅者 */
					this->notify_observable(clientfd, event_clientaccpet);

					/*将新的fd添加到epoll的监听队列中*/
					addepollfd_skt(epfd,clientfd,true);

					resetonshot_skt(epfd,s);
				}
			}
			else if (events[i].events & EPOLLIN)/*有可读文件*/
			{
				if (0 > sockfd)
					continue;

				char buf[65535];
				int datalen = receive_skt(sockfd, buf, sizeof(buf));
				if (0 == datalen)
				{
					continue;
				}
				else if (datalen == SOCKET_ERROR)
				{
					if (geterror_skt() != EAGAIN)
					{

						/*客户端套接字描述符从监听集合中删除*/
						delepollfd_skt(epfd, sockfd, true);

						clientclose(sockfd);
					}
					else
					{
						resetonshot_skt(epfd, sockfd);
						continue;
					}
					//break;
				}
				else
				{
					do
					{
						std::lock_guard<std::mutex> lock(_mutexdata);
						recvdata data;
						data.socket = sockfd;
						data.size = datalen;
						datalen = datalen > DATAPACKETSIZE ? DATAPACKETSIZE : datalen;
						memcpy(&data.buffer, buf, datalen);
						std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
						_dataqueue.push(recvda);
					} while (0);

					/* 通知订阅者 */
					this->notify_observable(sockfd, event_serverrecv);

					resetonshot_skt(epfd, sockfd);
				}
			}
		}
	}
#endif
}

void CMSSocket::accpet_skt(int s)
{
	struct sockaddr_in sockAddr;
	int addrlen = sizeof(sockAddr);
	while (true)
	{
		/*堵塞，等待客户端连接*/
		int sockfd = static_cast<int>(accept(s, (struct sockaddr *)&sockAddr, (socklen_t *)&addrlen));
		if (INVALID_SOCKET == sockfd)
		{
			break;
		}
		else
		{
			/*
			struct sockaddr_in clientAddr;
			int clientAddrLen = sizeof(clientAddr);
			//获取sockfd表示的连接上的本地地址
			getsockname(sockfd, (struct sockaddr*)&clientAddr, &clientAddrLen);

			int prot = ntohs(clientAddr.sin_port);
			std::string ip = inet_ntoa(clientAddr.sin_addr);
			std::string iP = inet_ntoa(sockAddr.sin_addr);
			*/

			/*添加到客户socket列表*/
			addclientsock(sockfd);

			/* 通知订阅者 */
			this->notify_observable(sockfd, event_clientaccpet);
			auto fun = std::bind(&CMSSocket::severreceive_skt, this, sockfd);
			_pthreadpool->append(fun);
		}
	}
	closesocket(s);
}

void CMSSocket::clientreceive_skt(int s)
{
	char	buf[65535];
	while (true)
	{
		int datalen = receive_skt(s, buf, sizeof(buf));
		if (datalen == 0)
		{
			continue;
		}
		else if (SOCKET_ERROR == datalen)
		{
			break;
		}
		else
		{
			do
			{
				std::lock_guard<std::mutex> lock(_mutexdata);
				recvdata data;
				data.socket = s;
				data.size = datalen;
				datalen = datalen > DATAPACKETSIZE ? DATAPACKETSIZE : datalen;
				memcpy(&data.buffer, buf, datalen);
				std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
				_dataqueue.push(recvda);
			} while (0);

			/* 通知订阅者 */
			this->notify_observable(s, event_clientrecv);
		}
	}
	clientclose(s);
}

void CMSSocket::severreceive_skt(int s)
{
	char	buf[65535];
	while (true)
	{
		int datalen = receive_skt(s, buf, sizeof(buf));
		if (0 == datalen)
		{
			continue;
		}
		else if (SOCKET_ERROR == datalen)
		{
			break;
		}
		else
		{
			do
			{
				std::lock_guard<std::mutex> lock(_mutexdata);
				recvdata data;
				data.socket = s;
				data.size = datalen;
				datalen = datalen > DATAPACKETSIZE ? DATAPACKETSIZE : datalen;
				memcpy(&data.buffer, buf, datalen);
				std::shared_ptr<recvdata> recvda = std::make_shared<recvdata>(data);
				_dataqueue.push(recvda);
			} while (0);


			/* 通知订阅者 */
			this->notify_observable(s, event_serverrecv);
		}
	}
	clientclose(s);
}

int CMSSocket::client_connect(std::string addr, int port)
{
	int s = make_skt();
	if (SOCKET_ERROR == s)
		return -1;
	
	if (!connect_skt(s, addr, port))
	{
		return -1;
	}

	auto func = std::bind(&CMSSocket::clientreceive_skt, this, s);
	_pthreadpool->append(func);

	return s;
}

int CMSSocket::sever_create(std::string addr, int port)
{
	int s = make_skt();
	if (SOCKET_ERROR == s)
		return false;

	if (!listen_skt(s, addr, port))
	{
		return false;
	}
	return s;
}

bool CMSSocket::get_recvbuf(int &socket, int &size, char **buffer)
{  
	if (0 == _dataqueue.size())
	{
		return false;
	}

	do{
		std::lock_guard<std::mutex> lock(_mutexdata);
		std::shared_ptr<recvdata> pdata;
		pdata = _dataqueue.front();
		_dataqueue.pop();
		socket = pdata->socket;
		size = pdata->size;
		memcpy(*buffer, pdata->buffer, size);
	} while (0);

	return true;
};