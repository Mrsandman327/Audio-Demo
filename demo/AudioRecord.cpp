#include "AudioRecord.h"
#include "AudioParameters.h"

static int inputCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	// 在此处处理音频数据
#if 0
	// 将输入缓冲区中的数据复制到输出缓冲区以回放声音
	if (inputBuffer != NULL)
		memcpy(outputBuffer, inputBuffer, framesPerBuffer * sizeof(DATA_TYPE));
#else
	CAudioRecord *pRecord = (CAudioRecord *)userData;
	if (nullptr != pRecord) {
#ifdef RECORD
			if (nullptr != pRecord->_audioencoder)
			{
				pRecord->_audioencoder->Mp3Encoder((DATA_TYPE*)inputBuffer, framesPerBuffer);
				pRecord->_audioencoder->WavEncoder((DATA_TYPE*)inputBuffer, framesPerBuffer);
			}
#endif
			pRecord->SendStream(inputBuffer, framesPerBuffer);
	}
#endif

	return paContinue;
}

CAudioRecord::CAudioRecord(CMSSocket *sub, int socket)
	: _socket(nullptr)
	, _instream(nullptr)
	, _outstream(nullptr)
#ifdef RECORD
	, _audioencoder(nullptr)
#endif
{
	_socket = sub;

	if(socket > 0)
		_client.push_back(socket);
#ifdef RECORD
	_audioencoder = new CAudioEncoder;
#endif

	OpenDefaultAudio();
}


CAudioRecord::~CAudioRecord()
{
	CloseCurrentAudio();
#ifdef RECORD
	if (nullptr != _audioencoder)
	{
		delete _audioencoder;
		_audioencoder = nullptr;
	}
#endif
}

bool CAudioRecord::OpenDefaultAudio()
{
	PaError err;

	// 初始化PortAudio库
	err = Pa_Initialize();
	if (err != paNoError) {
		printf("初始化PortAudio失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	//获取默认设备信息
	int defaultInputIndex = Pa_GetDefaultInputDevice();
	const   PaDeviceInfo *inputDeviceInfo = Pa_GetDeviceInfo(defaultInputIndex);
	int defaultOutputIndex = Pa_GetDefaultOutputDevice();
	const   PaDeviceInfo *outputDeviceInfo = Pa_GetDeviceInfo(defaultOutputIndex);

	PaStreamParameters inputParameters, outputParameters;
	inputParameters.device = defaultInputIndex;
	inputParameters.channelCount = CHANNELS_NUM;
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.suggestedLatency = inputDeviceInfo->defaultHighInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;

	outputParameters.device = defaultOutputIndex;
	outputParameters.channelCount = CHANNELS_NUM;
	outputParameters.sampleFormat = PA_SAMPLE_TYPE;
	outputParameters.suggestedLatency = outputDeviceInfo->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	double sampleRate = SAMPLE_RATE;
	if (inputParameters.channelCount > 0) {
		err = Pa_IsFormatSupported(&inputParameters, nullptr, sampleRate);
		if (err != paFormatIsSupported) {
			printf("默认输入设备不支持该采样率:%8.2f", sampleRate);
			return false;
		}
	}

	if (outputParameters.channelCount > 0) {
		err = Pa_IsFormatSupported(nullptr, &outputParameters, sampleRate);
		if (err != paFormatIsSupported) {
			printf("默认输出设备不支持该采样率:%8.2f", sampleRate);
			return false;
		}
	}

	//打开输入流
	err = Pa_OpenStream(
		&_instream,
		&inputParameters,
		nullptr,
		sampleRate,
		FRAMES_PER_BUFFER,
		paClipOff,
		inputCallback,
		this);
	if (err != paNoError) {
		printf("打开输入音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// 启动音频流
	err = Pa_StartStream(_instream);
	if (err != paNoError) {
		printf("启动音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	//打开输出流
	err = Pa_OpenStream(
		&_outstream,
		nullptr,
		&outputParameters,
		sampleRate,
		FRAMES_PER_BUFFER,
		paClipOff,
		nullptr,
		this);
	if (err != paNoError) {
		printf("打开出音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// 启动音频流
	err = Pa_StartStream(_outstream);
	if (err != paNoError) {
		printf("启动音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}
#ifdef RECORD
	if (nullptr != _audioencoder)
	{
		_audioencoder->Mp3Init("f:/linsn.mp3", SAMPLE_RATE, CHANNELS_NUM);
		_audioencoder->WavInit("f:/linsn.wav", SAMPLE_RATE, CHANNELS_NUM, CHANNELS_NUM * SAMPLE_SIZE * 8);
	}
#endif

	return true;
}

bool CAudioRecord::CloseCurrentAudio()
{
	PaError err;

	// 停止音频流
	err = Pa_StopStream(_instream);
	if (err != paNoError) {
		printf("停止音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// 关闭音频流和PortAudio库
	err = Pa_CloseStream(_instream);
	if (err != paNoError) {
		printf("关闭音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// 停止音频流
	err = Pa_StopStream(_outstream);
	if (err != paNoError) {
		printf("停止音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// 关闭音频流和PortAudio库
	err = Pa_CloseStream(_outstream);
	if (err != paNoError) {
		printf("关闭音频流失败: %s\n", Pa_GetErrorText(err));
		return false;
	}

	Pa_Terminate();
#ifdef RECORD
	if (nullptr != _audioencoder) {
		_audioencoder->Mp3Finsh();
		_audioencoder->WavFinsh();
	}
#endif

	return true;
}

bool CAudioRecord::SendStream(const void* inputBuffer, const int framesPerBuffer)
{
#if 0
	/*本地播放*/
	Pa_WriteStream(_outstream, inputBuffer, framesPerBuffer);
#else
	/*帧缓存*采样深度*通道数*/
	long buffersize = framesPerBuffer * SAMPLE_SIZE * CHANNELS_NUM;
	for (int i = 0; i < _client.size(); ++i)
	{
		char *buffer = new char[buffersize];
		memcpy(buffer, inputBuffer, buffersize);
		_socket->send_skt(_client[i], buffer, buffersize);
		delete[] buffer;
	}
#endif

	return true;
}

void CAudioRecord::RecvDataProcess()
{
	int clientsocket = 0;
	int datasize = 0;
	char *buffer = new char[DATAPACKETSIZE];
	if (_socket->get_recvbuf(clientsocket, datasize, &buffer))
	{
		long size = Pa_GetStreamWriteAvailable(_outstream);
		if (size * SAMPLE_SIZE >= datasize)
		{
			/*传输过来的数据是char类型（1个字节），根据采样格式将datasize切分*/
			Pa_WriteStream(_outstream, buffer, sizeof(char) * datasize / SAMPLE_SIZE / CHANNELS_NUM);
		}
		else {
			Pa_WriteStream(_outstream, buffer, sizeof(char) * size);
		}
	}
	delete[] buffer;
}

void CAudioRecord::Update(int socket, socketevent event)
{
	switch (event)
	{
	case event_unkown:printf("socket:%d 未知的事件\n", socket);
		break;
	case event_clientrecv:printf("socket:%d 客户端收到信息\n", socket);
		RecvDataProcess();
		break;
	case event_serverrecv:printf("socket:%d 服务器收到信息\n", socket);
		RecvDataProcess();
		break;
	case event_clientaccpet:printf("socket:%d 客户端连接\n", socket);
	{
		auto result = find(_client.cbegin(), _client.cend(), socket);
		if (_client.end() == result)
			_client.push_back(socket);
	}
		break;
	case event_clientdiscon:printf("socket:%d 客户端断开连接\n", socket);
	{
		auto result = find(_client.cbegin(), _client.cend(), socket);
		if (_client.end() != result)
			_client.erase(result);

	}
		break;
	case event_servercolse:printf("socket:%d 服务器关闭\n", socket);
		break;
	default:
		break;
	}
}

