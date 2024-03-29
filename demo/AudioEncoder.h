/**************************************************************************
*  @Copyright (c) 2024, linsn, All rights reserved.

*  @file     : Mp3Encoder.h
*  @version  : ver 1.0

*  @author   : linsn
*  @date     : 2024/01/18 19:42
*  @brief    : 音频编解码
**************************************************************************/
#pragma once
#include <string>
#include "lame.h"

class CAudioEncoder
{
public:
	CAudioEncoder();
	~CAudioEncoder();

	struct WAVE_INFO
	{
		typedef struct {
			char          ChunkID[4]; //内容为"RIFF"
			unsigned long ChunkSize;  //存储文件的字节数（不包含ChunkID和ChunkSize这8个字节）
			char          Format[4];  //内容为"WAVE“
		} WAVE_HEADER;

		typedef struct {
			char           Subchunk1ID[4]; //内容为"fmt"
			unsigned long  Subchunk1Size;  //存储该子块的字节数（不含前面的Subchunk1ID和Subchunk1Size这8个字节）
			unsigned short AudioFormat;    //存储音频文件的编码格式，例如若为PCM则其存储值为1。
			unsigned short NumChannels;    //声道数，单声道(Mono)值为1，双声道(Stereo)值为2，等等
			unsigned long  SampleRate;     //采样率，如8k，44.1k等
			unsigned long  ByteRate;       //每秒存储的bit数，其值 = SampleRate * NumChannels * BitsPerSample / 8
			unsigned short BlockAlign;     //块对齐大小，其值 = NumChannels * BitsPerSample / 8
			unsigned short BitsPerSample;  //每个采样点的bit数，一般为8,16,32等。
		} WAVE_FMT;

		typedef struct {
			char          Subchunk2ID[4]; //内容为“data”
			unsigned long Subchunk2Size;  //接下来的正式的数据部分的字节数，其值 = NumSamples * NumChannels * BitsPerSample / 8
		} WAVE_DATA;

		WAVE_HEADER wHeader;
		WAVE_FMT wFmt;
		WAVE_DATA wData;
	};

public:
	/*mp3 Encoder basic*/
	bool Mp3Init(const std::string file, unsigned int samplerate, unsigned int channels = 1, unsigned long buffersize = 0);
	bool Mp3Encoder(short* data, unsigned long frameCount);
	bool Mp3Encoder(float* data, unsigned long frameCount);
	bool Mp3Encoder(int* data, unsigned long frameCount);
	bool Mp3Encoder_Interleaved(short* data, unsigned long frameCount);
	bool Mp3Encoder_Interleaved(float* data, unsigned long frameCount);
	bool Mp3Encoder_Interleaved(int* data, unsigned long frameCount);
	bool Mp3Finsh();
	/*wav Encoder basic*/
	bool WavInit(const std::string file, unsigned int samplerate, unsigned int channels, unsigned int bitdepth = 16);
	bool WavEncoder(const void* data, unsigned long frameCount);
	bool WavFinsh();
	/*method*/
	bool GetWavHeaderInfo(const std::string wavfile, WAVE_INFO &info);
	bool SetWavHeaderInfo(const std::string pcmfile, WAVE_INFO info);
	bool SetWavHeaderInfo(const std::string pcmfile, unsigned int samplerate, unsigned int channels, unsigned int bitdepth = 16);
	bool WavToMp3(const std::string wavfile, const std::string mp3file);
	bool Mp3ToWav(const std::string mp3file, const std::string wavfile);

private:
	lame_global_flags* _flags;
	unsigned char* _mp3buf;
	FILE* _fmp3;
	unsigned long  _buffersize;

	FILE* _fwav;
	std::string _pcmfile;
	WAVE_INFO _wavinfo;

	void FreeMp3cache();
	void FreeWavcache();
};