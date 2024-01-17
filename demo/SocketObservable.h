#pragma once
enum socketevent
{
	event_unkown = 0x00000000,
	event_clientrecv = 0x00000001,
	event_serverrecv = 0x00000002,
	event_clientaccpet = 0x00000004,
	event_clientdiscon = 0x00000008,
	event_servercolse = 0x000000010,
};
class CSocketObservable
{
public:
	virtual void Update(int socket, socketevent event) = 0;
};

