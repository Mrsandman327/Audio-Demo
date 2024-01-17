#pragma once
#include <string>
#include "lame.h"

class CMp3Encoder
{
public:
	CMp3Encoder();
	~CMp3Encoder();

	struct WAVE_INFO
	{
		typedef struct {
			char          ChunkID[4]; //����Ϊ"RIFF"
			unsigned long ChunkSize;  //�洢�ļ����ֽ�����������ChunkID��ChunkSize��8���ֽڣ�
			char          Format[4];  //����Ϊ"WAVE��
		} WAVE_HEADER;

		typedef struct {
			char           Subchunk1ID[4]; //����Ϊ"fmt"
			unsigned long  Subchunk1Size;  //�洢���ӿ���ֽ���������ǰ���Subchunk1ID��Subchunk1Size��8���ֽڣ�
			unsigned short AudioFormat;    //�洢��Ƶ�ļ��ı����ʽ��������ΪPCM����洢ֵΪ1��
			unsigned short NumChannels;    //��������������(Mono)ֵΪ1��˫����(Stereo)ֵΪ2���ȵ�
			unsigned long  SampleRate;     //�����ʣ���8k��44.1k��
			unsigned long  ByteRate;       //ÿ��洢��bit������ֵ = SampleRate * NumChannels * BitsPerSample / 8
			unsigned short BlockAlign;     //������С����ֵ = NumChannels * BitsPerSample / 8
			unsigned short BitsPerSample;  //ÿ���������bit����һ��Ϊ8,16,32�ȡ�
		} WAVE_FMT;

		typedef struct {
			char          Subchunk2ID[4]; //����Ϊ��data��
			unsigned long Subchunk2Size;  //����������ʽ�����ݲ��ֵ��ֽ�������ֵ = NumSamples * NumChannels * BitsPerSample / 8
		} WAVE_DATA;

		WAVE_HEADER wHeader;
		WAVE_FMT wFmt;
		WAVE_DATA wData;
	};

public:
	/*Encoder basic*/
	bool Init(const std::string file, unsigned int samplerate, unsigned int channels = 1, unsigned long buffersize = 0);
	bool Encoder(short* data, unsigned long frameCount);
	bool Encoder(float* data, unsigned long frameCount);
	bool Encoder(int* data, unsigned long frameCount);
	bool Encoder_Interleaved(short* data, unsigned long frameCount);
	bool Encoder_Interleaved(float* data, unsigned long frameCount);
	bool Encoder_Interleaved(int* data, unsigned long frameCount);
	bool Finsh();
	/*method*/
	bool GetWavHeaderInfo(const std::string wavfile, WAVE_INFO &info);
	bool SetWavHeaderInfo(const std::string pcmfile, WAVE_INFO info);
	bool WavToMp3(const std::string wavfile, const std::string mp3file);
	bool Mp3ToWav(const std::string mp3file, const std::string wavfile);

private:
	lame_global_flags* _flags;
	unsigned char* _mp3buf;
	FILE* _fp;
	unsigned long  _buffersize;
	unsigned int _samplerate;
};

