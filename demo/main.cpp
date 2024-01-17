#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <iostream>
#include <fstream>
#include "MSSocket.h"
#include "AudioRecord.h"
#include "Mp3Encoder.h"

#define _SERVER		0	
#define _CLIENT		1   
#define _IOMODEL_  _SERVER

int main(int argc, char *argv[])
{

#if 01
	/*Mp3Encoder文件转化示例*/
	CMp3Encoder Mp3Encoder;

	Mp3Encoder.WavToMp3("f:/Alarm01.wav", "f:/724.mp3");
	Mp3Encoder.Mp3ToWav("f:/724.mp3", "f:/terst.wav");

	return 0;
#endif
	
#if _IOMODEL_ == _SERVER //服务器
	if (argc < 3) {
		printf("非法参数！");
		return 1;
	}
	/*创建被订阅者*/
	CMSSocket *socket = new CMSSocket;

	/*创建服务器*/
	int sockfd = socket->sever_create(argv[1], atoi(argv[2]));

	/*创建订阅者*/
	CAudioRecord *server = new CAudioRecord(socket);

	/*添加订阅者*/
	socket->attach_observable(server);

	/*堵塞*/
	getchar();

	/*退出*/
	socket->severclose(sockfd);
	socket->clear_observable();
	delete server;
	delete socket;
#else					//客户端
	/*创建被订阅者*/
	CMSSocket *socket = new CMSSocket;

	/*连接服务器*/
	int socketfd = socket->client_connect(argv[1], atoi(argv[2]));
	if (socketfd < 0) {
		printf("连接服务器失败！");
		return 0;
	}
	
	/*创建订阅者*/
	CAudioRecord *client = new CAudioRecord(socket, socketfd);

	/*添加订阅者*/
	socket->attach_observable(client);

	/*堵塞*/
	getchar();

	/*退出*/
	socket->clientclose(socketfd);
	socket->clear_observable();
	delete client;
	delete socket;
#endif
	return 0;
}