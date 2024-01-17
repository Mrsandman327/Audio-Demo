#include "AudioRecord.h"
#include "AudioParameters.h"

static int inputCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void* userData)
{
	// �ڴ˴�������Ƶ����
#if 0
	// �����뻺�����е����ݸ��Ƶ�����������Իط�����
	if (inputBuffer != NULL)
		memcpy(outputBuffer, inputBuffer, framesPerBuffer * sizeof(DATA_TYPE));
#else
	CAudioRecord *pRecord = (CAudioRecord *)userData;
	if (nullptr != pRecord) {
#ifdef RECORD
			if (nullptr != pRecord->_mp3encoder)
			{
				pRecord->_mp3encoder->Encoder((DATA_TYPE*)inputBuffer, framesPerBuffer);
			}
			if (nullptr != pRecord->_fwav)
			{
				fwrite((DATA_TYPE*)inputBuffer, SAMPLE_SIZE, framesPerBuffer, pRecord->_fwav);
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
	, _mp3encoder(nullptr)
	, _fwav(nullptr)
#endif
{
	_socket = sub;

	if(socket > 0)
		_client.push_back(socket);
#ifdef RECORD
	_mp3encoder = new CMp3Encoder;
#endif

	OpenDefaultAudio();
}


CAudioRecord::~CAudioRecord()
{
	CloseCurrentAudio();
#ifdef RECORD
	if (nullptr != _mp3encoder)
	{
		delete _mp3encoder;
		_mp3encoder = nullptr;
	}
#endif
}

bool CAudioRecord::OpenDefaultAudio()
{
	PaError err;

	// ��ʼ��PortAudio��
	err = Pa_Initialize();
	if (err != paNoError) {
		printf("��ʼ��PortAudioʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	//��ȡĬ���豸��Ϣ
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
			printf("Ĭ�������豸��֧�ָò�����:%8.2f", sampleRate);
			return false;
		}
	}

	if (outputParameters.channelCount > 0) {
		err = Pa_IsFormatSupported(nullptr, &outputParameters, sampleRate);
		if (err != paFormatIsSupported) {
			printf("Ĭ������豸��֧�ָò�����:%8.2f", sampleRate);
			return false;
		}
	}

	//��������
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
		printf("��������Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// ������Ƶ��
	err = Pa_StartStream(_instream);
	if (err != paNoError) {
		printf("������Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	//�������
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
		printf("�򿪳���Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// ������Ƶ��
	err = Pa_StartStream(_outstream);
	if (err != paNoError) {
		printf("������Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}
#ifdef RECORD
	if(nullptr != _mp3encoder)
		_mp3encoder->Init("f:/linsn.mp3", SAMPLE_RATE, CHANNELS_NUM);

	_fwav = fopen("f:/linsn.wav", "wb+");
#endif

	return true;
}

bool CAudioRecord::CloseCurrentAudio()
{
	PaError err;

	// ֹͣ��Ƶ��
	err = Pa_StopStream(_instream);
	if (err != paNoError) {
		printf("ֹͣ��Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// �ر���Ƶ����PortAudio��
	err = Pa_CloseStream(_instream);
	if (err != paNoError) {
		printf("�ر���Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// ֹͣ��Ƶ��
	err = Pa_StopStream(_outstream);
	if (err != paNoError) {
		printf("ֹͣ��Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	// �ر���Ƶ����PortAudio��
	err = Pa_CloseStream(_outstream);
	if (err != paNoError) {
		printf("�ر���Ƶ��ʧ��: %s\n", Pa_GetErrorText(err));
		return false;
	}

	Pa_Terminate();
#ifdef RECORD
	if (nullptr != _mp3encoder)
		_mp3encoder->Finsh();

	/*����wav�ļ�ͷ*/
	fseek(_fwav, 0, SEEK_END);
	CMp3Encoder::WAVE_INFO info;
	memcpy(info.wHeader.ChunkID, "RIFF", sizeof(char) * 4);
	memcpy(info.wHeader.Format, "WAVE", sizeof(char) * 4);
	info.wHeader.ChunkSize = ftell(_fwav) + 44 - 8;
	memcpy(info.wFmt.Subchunk1ID, "fmt ", sizeof(char) * 4);
	info.wFmt.AudioFormat = 1;
	info.wFmt.NumChannels = CHANNELS_NUM;
	info.wFmt.SampleRate = SAMPLE_RATE;
	info.wFmt.BitsPerSample = SAMPLE_SIZE * 8;
	info.wFmt.BlockAlign = info.wFmt.NumChannels * info.wFmt.BitsPerSample / 8;
	info.wFmt.ByteRate = info.wFmt.SampleRate * info.wFmt.BlockAlign;
	info.wFmt.Subchunk1Size = 16;
	memcpy(info.wData.Subchunk2ID, "data ", sizeof(char) * 4);
	info.wData.Subchunk2Size = ftell(_fwav);
	fclose(_fwav);
	_fwav = nullptr;

	_mp3encoder->SetWavHeaderInfo("f:/linsn.wav", info);
#endif

	return true;
}

bool CAudioRecord::SendStream(const void* inputBuffer, const int framesPerBuffer)
{
#if 0
	/*���ز���*/
	Pa_WriteStream(_outstream, inputBuffer, framesPerBuffer);
#else
	/*֡����*�������*ͨ����*/
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
			/*���������������char���ͣ�1���ֽڣ������ݲ�����ʽ��datasize�з�*/
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
	case event_unkown:printf("socket:%d δ֪���¼�\n", socket);
		break;
	case event_clientrecv:printf("socket:%d �ͻ����յ���Ϣ\n", socket);
		RecvDataProcess();
		break;
	case event_serverrecv:printf("socket:%d �������յ���Ϣ\n", socket);
		RecvDataProcess();
		break;
	case event_clientaccpet:printf("socket:%d �ͻ�������\n", socket);
	{
		auto result = find(_client.cbegin(), _client.cend(), socket);
		if (_client.end() == result)
			_client.push_back(socket);
	}
		break;
	case event_clientdiscon:printf("socket:%d �ͻ��˶Ͽ�����\n", socket);
	{
		auto result = find(_client.cbegin(), _client.cend(), socket);
		if (_client.end() != result)
			_client.erase(result);

	}
		break;
	case event_servercolse:printf("socket:%d �������ر�\n", socket);
		break;
	default:
		break;
	}
}

