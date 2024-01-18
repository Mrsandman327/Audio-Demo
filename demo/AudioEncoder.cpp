#include "AudioEncoder.h"

CAudioEncoder::CAudioEncoder()
	: _flags(nullptr)
	, _mp3buf(nullptr)
	, _fmp3(nullptr)
	, _fwav(nullptr)
	, _buffersize(0)
	, _pcmfile("")
{
}

CAudioEncoder::~CAudioEncoder()
{
	FreeMp3cache();
}

bool CAudioEncoder::Mp3Init(const std::string file, unsigned int samplerate, unsigned int channels, unsigned long buffersize /*= 0*/)
{
	FreeMp3cache();

	_fmp3 = fopen(file.c_str(), "wb+");
	if (nullptr == _fmp3)
		return false;

	/*��ʼ��,Ĭ�ϸ�ʽ��J-Stereo, 44.1khz 128kbps CBR quality=5*/
	_flags = lame_init();
	if (nullptr == _flags)
		return false;
	/*�������������*/
	lame_set_in_samplerate(_flags, samplerate);
	lame_set_out_samplerate(_flags, samplerate);
	/*����ͨ����*/
	lame_set_num_channels(_flags, channels);
	/*�������ģʽ MONO:��ͨ�� STEREO:˫ͨ��*/
	lame_set_mode(_flags, MONO);
	/*���ñ����ʿ���ģʽ*/
	lame_set_VBR(_flags, vbr_default);
	/*����CBR�ı�����*/
	lame_set_brate(_flags, 128);  
	/* 2=high 5 = medium 7=low ����*/
	lame_set_quality(_flags, 5);
	/*����������*/							  
	int ret = lame_init_params(_flags);
	if (ret < 0)
		return false;

	/*mp3�������� ��С�Ƽ��������� / 20 +7200*/
	_buffersize = buffersize > samplerate / 20 + 7200 ? buffersize : samplerate / 20 + 7200;
	_mp3buf = (unsigned char*)malloc(_buffersize);

	return true;
}

bool CAudioEncoder::Mp3Encoder(short* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf	||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;
	/*PCM���ݱ���ΪMP3����*/
	int mp3bytes = lame_encode_buffer(_flags, data, nullptr, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;
	
	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	return true;
}

bool CAudioEncoder::Mp3Encoder(float* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;

	/*PCM���ݱ���ΪMP3����, �����float�����ǹ�һ��������*/
	int mp3bytes = lame_encode_buffer_ieee_float(_flags, data, nullptr, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	return true;
}

bool CAudioEncoder::Mp3Encoder(int* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;
	/*PCM���ݱ���ΪMP3����*/
	int mp3bytes = lame_encode_buffer_int(_flags, data, nullptr, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	return true;
}

bool CAudioEncoder::Mp3Encoder_Interleaved(short* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;
	/*PCM���ݱ���ΪMP3����*/
	int mp3bytes = lame_encode_buffer_interleaved(_flags, data, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	return true;
}

bool CAudioEncoder::Mp3Encoder_Interleaved(float* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;

	/*PCM���ݱ���ΪMP3����, �����float�����ǹ�һ��������*/
	int mp3bytes = lame_encode_buffer_interleaved_ieee_float(_flags, data, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	return true;
}

bool CAudioEncoder::Mp3Encoder_Interleaved(int* data, unsigned long frameCount)
{
	if (nullptr == _mp3buf ||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;
	/*PCM���ݱ���ΪMP3����*/
	int mp3bytes = lame_encode_buffer_interleaved_int(_flags, data, frameCount, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;

	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	return true;
}

bool CAudioEncoder::Mp3Finsh()
{
	if (nullptr == _mp3buf ||
		nullptr == _fmp3 ||
		nullptr == _flags)
		return false;
	/*ˢ�±��������壬��ȡ�����ڱ����������������*/
	int mp3bytes = lame_encode_flush(_flags, _mp3buf, _buffersize);
	if (mp3bytes <= 0)
		return false;
	fwrite(_mp3buf, 1, mp3bytes, _fmp3);

	/*д��VBRTAG*/
	lame_mp3_tags_fid(_flags, _fmp3);

	/*���ٱ��������ͷ���Դ*/
	FreeMp3cache();

	return true;
}

bool CAudioEncoder::WavInit(const std::string file, unsigned int samplerate, unsigned int channels, unsigned int bitdepth /*= 16*/)
{
	FreeWavcache();

	_fwav = fopen(file.c_str(), "wb+");
	if (nullptr == _fwav)
		return false;

	_pcmfile = file;

	/*�����ļ�ͷ*/
	memcpy(_wavinfo.wHeader.ChunkID, "RIFF", sizeof(char) * 4);
	memcpy(_wavinfo.wHeader.Format, "WAVE", sizeof(char) * 4);
	_wavinfo.wHeader.ChunkSize = 0;
	memcpy(_wavinfo.wFmt.Subchunk1ID, "fmt ", sizeof(char) * 4);
	_wavinfo.wFmt.AudioFormat = 1;
	_wavinfo.wFmt.NumChannels = channels;
	_wavinfo.wFmt.SampleRate = samplerate;
	_wavinfo.wFmt.BitsPerSample = bitdepth;
	_wavinfo.wFmt.BlockAlign = _wavinfo.wFmt.NumChannels * _wavinfo.wFmt.BitsPerSample / 8;
	_wavinfo.wFmt.ByteRate = _wavinfo.wFmt.SampleRate * _wavinfo.wFmt.BlockAlign;
	_wavinfo.wFmt.Subchunk1Size = sizeof(WAVE_INFO::WAVE_FMT) - 8;
	memcpy(_wavinfo.wData.Subchunk2ID, "data ", sizeof(char) * 4);
	_wavinfo.wData.Subchunk2Size = 0;

	return true;
}

bool CAudioEncoder::WavEncoder(const void* data, unsigned long frameCount)
{
   	if (nullptr == _fwav ||
		0 == _wavinfo.wFmt.BlockAlign)
		return false;

	fwrite(data, sizeof(char) * _wavinfo.wFmt.BlockAlign, frameCount, _fwav);
	
	return true;
}

bool CAudioEncoder::WavFinsh()
{
	if (nullptr == _fwav)
		return false;

	fseek(_fwav, 0, SEEK_END);
	_wavinfo.wHeader.ChunkSize = ftell(_fwav) + sizeof(WAVE_INFO) - 8;
	_wavinfo.wData.Subchunk2Size = ftell(_fwav);
	fclose(_fwav);
	_fwav = nullptr;

	if (!SetWavHeaderInfo(_pcmfile, _wavinfo))
		return false;

	return true;
}

bool CAudioEncoder::WavToMp3(const std::string wavfile, const std::string mp3file)
{
	/*�����ļ�ͷ*/
	WAVE_INFO wavinfo;
	if (!GetWavHeaderInfo(wavfile.c_str(), wavinfo))
		return false;

	FILE *fwav = fopen(wavfile.c_str(), "rb");
	if (nullptr == fwav)
		return false;
	fseek(fwav, 44, SEEK_CUR);

	if (!Mp3Init(mp3file, wavinfo.wFmt.SampleRate, wavinfo.wFmt.NumChannels, 8192 * wavinfo.wFmt.BlockAlign))
		return false;

	char *wav_buffer = new char[8192 * wavinfo.wFmt.BlockAlign];
	int read = 0;
	do {
		/*����wav�ļ�������С��ÿ�����ݵ��ֽڳ��ȣ���ȡ����*/
		read = fread(wav_buffer, sizeof(char) * wavinfo.wFmt.BlockAlign, 8192, fwav);
		if (read != 0) {
			if( 1 >= wavinfo.wFmt.NumChannels)
				Mp3Encoder((short*)wav_buffer, read);/*��ͨ��*/
			else
				Mp3Encoder_Interleaved((short*)wav_buffer, read);/*˫ͨ��*/
		}
		else {
			Mp3Finsh();
		}
	} while (read != 0);
	delete[] wav_buffer;

	fclose(fwav);

	return true;
}

bool CAudioEncoder::Mp3ToWav(const std::string mp3file, const std::string wavfile)
{
	FreeMp3cache();

	/*��ʼ��*/
	auto lamefg = lame_init();
	if (nullptr == lamefg)
		return false;
	lame_set_decode_only(lamefg, 1);
	auto hip = hip_decode_init();
	if (nullptr == hip)
		return false;

	/*��MP3�ļ�*/
	FILE *fmp3 = fopen(mp3file.c_str(), "rb");
	if (nullptr == fmp3)
		return false;

	/*ѭ�����ļ��ж�ȡ16���ֽڳ���,����֡ͷ��
	���ɹ�������һ��֡ͷ������ʱ��ʵ���Ͼ͵õ�������MP3�Ĳ����ʺ�ͨ����*/
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
	/*�����ļ�ͷʧ��*/
	if (!mp3data.header_parsed)
	{
		fclose(fmp3);
		return false;
	}
	
	/*���ļ�ָ�벦�����*/
	fseek(fmp3, 0, SEEK_SET);
       
	/*
	����MP3
	����е����⣬������ȫ����htp: bitstream problem, resyncing skipping xxxx byte...��
	��ŵ���˼�����ҵ���һ֡֡ͷ��֮ǰ�����˶��ٸ��ַ����ݣ�һ����˵����һ���Σ��ϰٸ��ַ���ûɶ����
	��������Ĭ��λ���16λ�����ֱ�Ӳ���short
	*/
	if (!WavInit(wavfile, mp3data.samplerate, mp3data.stereo, 16))
		return false; 
	do 
	{
		read = fread(mp3buff, sizeof(char), 512, fmp3);
		if (read > 0) {
			/*iread���ص�ʱ��������*/
			int iread = hip_decode(hip, (unsigned char*)mp3buff, read, pcm_l, pcm_r);
			if (iread > 0)
			{
				short* data = new short[iread * mp3data.stereo];
				unsigned int j = 0;
				for (int i = 0; i < iread; i++) 
				{ 
					//˫ͨ���Ļ��ͽ������㽻��ϲ���һ��
					memcpy(data + j, pcm_l + i, 2);
					j += 1;
					if (mp3data.stereo == 2) {
						memcpy(data + j, pcm_r + i, 2);
						j += 1;
					}
				}
				//fwrite(data, sizeof(short)*mp3data.stereo, iread, fwav);
				WavEncoder(data, iread);
				delete[] data;
			}
		}
	} while (read > 0);

	hip_decode_exit(hip);
	fclose(fmp3);

	if (!WavFinsh())
		return false;
	
	return true;
}

bool CAudioEncoder::GetWavHeaderInfo(const std::string wavfile, WAVE_INFO &info)
{
	FILE *fwav = fopen(wavfile.c_str(), "rb");
	if (nullptr == fwav)
		return false;

	/*�����ļ�ͷ*/
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

bool CAudioEncoder::SetWavHeaderInfo(const std::string pcmfile, WAVE_INFO info)
{
	/*�����ļ�*/
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
	/*д���ļ�ͷ*/
	fwrite(&info, sizeof(char), sizeof(WAVE_INFO), fpcm);

	/*��ȡԭ���ݣ�д��ȥ*/
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

	/*ɾ����ʱ�ļ�*/
	std::remove(filebak.c_str());

	return true;
}

bool CAudioEncoder::SetWavHeaderInfo(const std::string pcmfile, unsigned int samplerate, unsigned int channels, unsigned int bitdepth)
{
	FILE *fwav = fopen(pcmfile.c_str(), "rb");
	if (nullptr == fwav)
		return false;

	fseek(fwav, 0, SEEK_END);
	WAVE_INFO info;
	memcpy(info.wHeader.ChunkID, "RIFF", sizeof(char) * 4);
	memcpy(info.wHeader.Format, "WAVE", sizeof(char) * 4);
	info.wHeader.ChunkSize = ftell(fwav) + sizeof(WAVE_INFO) - 8;
	memcpy(info.wFmt.Subchunk1ID, "fmt ", sizeof(char) * 4);
	info.wFmt.AudioFormat = 1;
	info.wFmt.NumChannels = channels;
	info.wFmt.SampleRate = samplerate;
	info.wFmt.BitsPerSample = bitdepth;
	info.wFmt.BlockAlign = info.wFmt.NumChannels * info.wFmt.BitsPerSample / 8;
	info.wFmt.ByteRate = info.wFmt.SampleRate * info.wFmt.BlockAlign;
	info.wFmt.Subchunk1Size = sizeof(WAVE_INFO::WAVE_FMT) - 8;
	memcpy(info.wData.Subchunk2ID, "data ", sizeof(char) * 4);
	info.wData.Subchunk2Size = ftell(fwav);
	fclose(fwav);

	return SetWavHeaderInfo(pcmfile, info);
}

void CAudioEncoder::FreeMp3cache()
{
	if (nullptr != _flags)
	{
		lame_close(_flags);
		_flags = nullptr;
	}
	if (nullptr != _fmp3)
	{
		fclose(_fmp3);
		_fmp3 = nullptr;
	}
	if (nullptr != _mp3buf)
	{
		free(_mp3buf);
		_mp3buf = nullptr;
	}
}

void CAudioEncoder::FreeWavcache()
{
	if (nullptr != _fwav)
	{
		fclose(_fwav);
		_fwav = nullptr;
	}
	if ("" != _pcmfile)
	{
		_pcmfile = "";
	}
	memset(&_wavinfo, 0, sizeof(WAVE_INFO));
}
