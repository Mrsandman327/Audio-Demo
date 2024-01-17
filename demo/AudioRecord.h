#pragma once
#include "MSSocket.h"
#include "SocketObservable.h"
#include "Mp3Encoder.h"
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
	CMp3Encoder *_mp3encoder;
	FILE *_fwav;
#endif
private:
	CMSSocket* _socket;
	std::vector<int> _client;
	void* _instream;
	void* _outstream;
	
	void RecvDataProcess();
	void Update(int socket, socketevent event) override;
};

