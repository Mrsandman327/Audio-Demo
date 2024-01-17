#include "Mp3Encoder.h"
#include <climits>
#include <cfloat>
#include <filesystem>

CMp3Encoder::CMp3Encoder()
	: _flags(nullptr)
	, _mp3buf(nullptr)
	, _fp(nullptr)
	, _samplerate(0)
	, _buffersize(0)
{
}

CMp3Encoder::~CMp3Encoder()
{
	if (nullptr != _flags) {
		lame_close(_flags);
		_flags = nullptr;
	}
	if (nullptr != _fp)	{
		fclose(_fp);
		_fp = nullptr;
	}
	if (nullptr != _mp3buf)
	{
		free(_mp3buf);
		_mp3buf = nullptr;
	}
}

bool CMp3Encoder::Init(const std::string file, unsigned int samplerate, unsigned int channels, unsigned long buffersize)
{
	if (nullptr != _flags)
	{
		lame_close(_flags);
		_flags = nullptr;
	}
	if (nullptr != _fp)
	{
		fclose(_fp);
		_fp = nullptr;
	}
	if (nullptr != _mp3buf)
	{
		free(_mp3buf);
		_mp3buf = nullptr;
	}

	_samplerate = samplerate;

	_fp = fopen(file.c_str(), "wb+");
	if (nullptr == _fp)
		return false;

	/*初始化,默认格式：J-Stereo, 44.1khz 128kbps CBR quality=5*/
	_flags = lame_init();
	if (nullptr == _flags)
		return false;
	/*设置输入采样率*/
	lame_set_in_samplerate(_flags, samplerate);
	lame_set_out_samplerate(_flags, samplerate);
	/*设置通道数*/
	lame_set_num_channels(_flags, channels);
	/*设置输出模式 MONO:单通道 STEREO:双通道*/
	lame_set_mode(_flags, MONO);
	/*设置比特率控制模式*/
	lame_set_VBR(_flags, vbr_default);
	/*设置CBR的比特率*/
	lame_set_brate(_flags, 128);  
	/* 2=high 5 = medium 7=low 音质*/
	lame_set_quality(_flags, 5);
	/*建立编码器*/							  
	int ret = lame_init_params(_flags);
	if (ret < 0)
		return false;

	/*mp3编码数据 大小推荐：采样率 / 20 +7200*/
	_buffersize = buffersize > samplerate / 20 + 7200 ? buffersize : samplerate / 20 + 7200;
	_mp3buf = (unsigned char*)malloc(_buffersize);

	return true;
}

bool CMp3Encoder::Encoder(short* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf	||
		nullptr == _fp		||
		nullptr == _flags	||
		0 >= _samplerate)
		return false;
	/*PCM数据编码为MP3数据*/
	int mp3bytes = lame_encode_buffer(_flags, data, nullptr, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;
	
	fwrite(_mp3buf, 1, mp3bytes, _fp);

	return true;
}

bool CMp3Encoder::Encoder(float* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fp ||
		nullptr == _flags ||
		0 >= _samplerate)
		return false;

	/*PCM数据编码为MP3数据, 这里的float数据是归一化的数据*/
	int mp3bytes = lame_encode_buffer_ieee_float(_flags, data, nullptr, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fp);

	return true;
}

bool CMp3Encoder::Encoder(int* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fp ||
		nullptr == _flags ||
		0 >= _samplerate)
		return false;
	/*PCM数据编码为MP3数据*/
	int mp3bytes = lame_encode_buffer_int(_flags, data, nullptr, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fp);

	return true;
}

bool CMp3Encoder::Encoder_Interleaved(short* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fp ||
		nullptr == _flags ||
		0 >= _samplerate)
		return false;
	/*PCM数据编码为MP3数据*/
	int mp3bytes = lame_encode_buffer_interleaved(_flags, data, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fp);

	return true;
}

bool CMp3Encoder::Encoder_Interleaved(float* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fp ||
		nullptr == _flags ||
		0 >= _samplerate)
		return false;

	/*PCM数据编码为MP3数据, 这里的float数据是归一化的数据*/
	int mp3bytes = lame_encode_buffer_interleaved_ieee_float(_flags, data, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fp);

	return true;
}

bool CMp3Encoder::Encoder_Interleaved(int* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fp ||
		nullptr == _flags ||
		0 >= _samplerate)
		return false;
	/*PCM数据编码为MP3数据*/
	int mp3bytes = lame_encode_buffer_interleaved_int(_flags, data, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fp);

	return true;
}

bool CMp3Encoder::Finsh()
{
	if (nullptr == _mp3buf ||
		nullptr == _fp ||
		nullptr == _flags ||
		0 >= _samplerate)
		return false;
	/*刷新编码器缓冲，获取残留在编码器缓冲里的数据*/
	int mp3bytes = lame_encode_flush(_flags, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;
	fwrite(_mp3buf, 1, mp3bytes, _fp);

	/*写入VBRTAG*/
	lame_mp3_tags_fid(_flags, _fp);

	/*销毁编码器，释放资源*/
	lame_close(_flags);
	_flags = nullptr;
	fclose(_fp);
	_fp = nullptr;
	free(_mp3buf);
	_mp3buf = nullptr;

	return true;
}

bool CMp3Encoder::WavToMp3(const std::string wavfile, const std::string mp3file)
{
	/*解析文件头*/
	WAVE_INFO wavinfo;
	if (!GetWavHeaderInfo(wavfile.c_str(), wavinfo))
		return false;

	FILE *fwav = fopen(wavfile.c_str(), "rb");
	if (nullptr == fwav)
		return false;
	fseek(fwav, 44, SEEK_CUR);

	Init(mp3file, wavinfo.wFmt.SampleRate, wavinfo.wFmt.NumChannels, 8192 * wavinfo.wFmt.BlockAlign);

	char *wav_buffer = new char[8192 * wavinfo.wFmt.BlockAlign];
	int read = 0;
	do {
		/*根据wav文件块对齐大小（每个数据的字节长度）读取数据*/
		read = fread(wav_buffer, sizeof(char) * wavinfo.wFmt.BlockAlign, 8192, fwav);
		if (read != 0) {
			if( 1 >= wavinfo.wFmt.NumChannels)
				Encoder((short*)wav_buffer, read);/*单通道*/
			else
				Encoder_Interleaved((short*)wav_buffer, read);/*双通道*/
		}
		else {
			Finsh();
		}
	} while (read != 0);
	delete[] wav_buffer;

	fclose(fwav);

	return true;
}

bool CMp3Encoder::Mp3ToWav(const std::string mp3file, const std::string wavfile)
{
	if (nullptr != _flags)
	{
		lame_close(_flags);
		_flags = nullptr;
	}
	if (nullptr != _fp)
	{
		fclose(_fp);
		_fp = nullptr;
	}
	if (nullptr != _mp3buf)
	{
		free(_mp3buf);
		_mp3buf = nullptr;
	}

	/*初始化*/
	auto lamefg = lame_init();
	if (nullptr == lamefg)
		return false;
	lame_set_decode_only(lamefg, 1);
	auto hip = hip_decode_init();
	if (nullptr == hip)
		return false;

	/*打开MP3文件*/
	FILE *fmp3 = fopen(mp3file.c_str(), "rb");
	if (nullptr == fmp3)
		return false;

	/*循环从文件中读取16个字节出来,解析帧头，
	当成功解析出一个帧头的数据时，实际上就得到了整个MP3的采样率和通道数*/
	short int pcm_l[9000];
	short int pcm_r[9000];
	void *mp3buff = new unsigned char[512];
	mp3data_struct mp3data;
	int read = 0;
	do 
	{	
		read = fread(mp3buff, sizeof(char), 16, fmp3);
		if (read > 0) {
			hip_decode_headers(hip, (unsigned char*)mp3buff, read, pcm_l, pcm_r, &mp3data);
		}
	} while (!mp3data.header_parsed && read > 0);
	/*解析文件头失败*/
	if (!mp3data.header_parsed)
	{
		fclose(fmp3);
		return false;
	}
	
	/*将文件指针拨回起点*/
	fseek(fmp3, 0, SEEK_SET);

	FILE *fwav = fopen(wavfile.c_str(), "wb");
	if (nullptr == fmp3)
		return false;

	/*
	解码MP3
	这个有点问题，解析不全报错：htp: bitstream problem, resyncing skipping xxxx byte...）
	大概的意思就是找到下一帧帧头，之前跳过了多少个字符数据，一般来说跳过一俩次，上百个字符都没啥问题
	另外这里默认位深度16位，因此直接采用short
	*/
	do 
	{
		read = fread(mp3buff, sizeof(char), 512, fmp3);
		if (read > 0) {
			/*iread返回的时采样个数*/
			int iread = hip_decode(hip, (unsigned char*)mp3buff, read, pcm_l, pcm_r);
			if (iread > 0)
			{
				short* data = new short[iread * mp3data.stereo];
				unsigned int j = 0;
				for (int i = 0; i < iread; i++) 
				{ 
					//双通道的话就将采样点交叉合并在一起。
					memcpy(data + j, pcm_l + i, 2);
					j += 1;
					if (mp3data.stereo == 2) {
						memcpy(data + j, pcm_r + i, 2);
						j += 1;
					}
				}
				fwrite(data, sizeof(short)*mp3data.stereo, iread, fwav);
				delete[] data;
			}
		}
	} while (read > 0);

	hip_decode_exit(hip);

	/*设置wav文件头*/
	fseek(fwav,0,SEEK_END);
	WAVE_INFO info;
	memcpy(info.wHeader.ChunkID, "RIFF", sizeof(char) * 4);
	memcpy(info.wHeader.Format, "WAVE", sizeof(char) * 4);
	info.wHeader.ChunkSize = ftell(fwav) + 44 - 8;
	memcpy(info.wFmt.Subchunk1ID, "fmt ", sizeof(char) * 4);
	info.wFmt.AudioFormat = 1;
	info.wFmt.NumChannels = mp3data.stereo;
	info.wFmt.SampleRate = mp3data.samplerate;
	info.wFmt.BitsPerSample = 16;
	info.wFmt.BlockAlign = info.wFmt.NumChannels * info.wFmt.BitsPerSample / 8;
	info.wFmt.ByteRate = info.wFmt.SampleRate * info.wFmt.BlockAlign;
	info.wFmt.Subchunk1Size = 16;
	memcpy(info.wData.Subchunk2ID, "data ", sizeof(char) * 4);
	info.wData.Subchunk2Size = ftell(fwav);
	fclose(fwav);
	SetWavHeaderInfo(wavfile,info);

	fclose(fmp3);

	return true;
}

bool CMp3Encoder::GetWavHeaderInfo(const std::string wavfile, WAVE_INFO &info)
{
	FILE *fwav = fopen(wavfile.c_str(), "rb");
	if (nullptr == fwav)
		return false;

	/*解析文件头*/
	unsigned char head[44];
	int sz = fread(head, sizeof(unsigned char), 44, fwav);

	if (44 != sz)
	{
		fclose(fwav);
		return false;
	}
		
	memcpy(&info, head, 44);
	fclose(fwav);

	return true;
}

bool CMp3Encoder::SetWavHeaderInfo(const std::string pcmfile, WAVE_INFO info)
{
	/*备份文件*/
	std::string filebak = pcmfile + ".bak";
#ifdef _WIN32
	/*copy "xxx.wav" "xxx.wav.bak"*/
	std::string cpcmd = "copy \"" + pcmfile + "\" \"" + filebak + "\"";
#else
	std::string cpcmd = "cp " + pcmfile + " " + filebak;
#endif
	if (int ret = system(cpcmd.c_str()) != 0)
		return false;

	FILE *fpcm = fopen(pcmfile.c_str(), "wb");
	if (nullptr == fpcm)
		return false;
	/*写入文件头*/
	fwrite(&info, sizeof(char), sizeof(WAVE_INFO), fpcm);

	/*读取原数据，写回去*/
	FILE *fbak = fopen(filebak.c_str(), "rb");
	if (nullptr == fbak)
	{
		fclose(fpcm);
		return false;
	}
	int read = 0;
	char buffer[1024];
	do 
	{
		read = fread(buffer, sizeof(char), 1024, fbak);
		if (read != 0)
		{
			fwrite(buffer, sizeof(char), 1024, fpcm);
		}
	} while (read != 0);
	fclose(fbak);
	fclose(fpcm);

	/*删除临时文件*/
	std::remove(filebak.c_str());

	return true;
}
