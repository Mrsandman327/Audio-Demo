/************************************************************************** 
    *  @Copyright (c) 2024, linsn, All rights reserved. 
 
    *  @file     : AudioRecord.h 
    *  @version  : ver 1.0 
 
    *  @author   : linsn 
    *  @date     : 2024/01/18 19:42 
    *  @brief    : ÒôÆµ²Ù×÷ 
**************************************************************************/  
#pragma once
#include "MSSocket.h"
#include "SocketObservable.h"
#include "AudioEncoder.h"
#define RECORD
class CAudioRecord: public CSocketObservable
{
public:
	CAudioRecord(CMSSocket *sub, int socket = -1);
	~CAudioRecord();

	bool OpenDefaultAudio();
	bool CloseCurrentAudio();
	bool SendStream(const void* inputBuffer, const int framesPerBuffer);
#ifdef RECORD
	CAudioEncoder *_audioencoder;
#endif
private:
	CMSSocket* _socket;
	std::vector<int> _client;
	void* _instream;
	void* _outstream;
	
	void RecvDataProcess();
	void Update(int socket, socketevent event) override;
};

