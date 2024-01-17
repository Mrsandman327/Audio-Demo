#pragma once
#include "portaudio.h"
/*采样率*/
#define SAMPLE_RATE       (44100)
/*帧缓存*/
#define FRAMES_PER_BUFFER   (512) 
/*通道*/
#define CHANNELS_NUM	(1)

/* Select 采样格式. */
#if 0 /*MP3 ok wav no*/
#define PA_SAMPLE_TYPE  paFloat32
#define DATA_TYPE float
#define SAMPLE_SIZE (4)
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 01 /*MP3 ok wav ok*/
#define PA_SAMPLE_TYPE  paInt16
#define DATA_TYPE short
#define SAMPLE_SIZE (2)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0 /*MP3 no wav ok*/
#define PA_SAMPLE_TYPE  paInt24
#define DATA_TYPE int
#define SAMPLE_SIZE (3)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0 /*MP3 no wav no*/
#define PA_SAMPLE_TYPE  paInt8
#define DATA_TYPE char
#define SAMPLE_SIZE (1)
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else 
#define PA_SAMPLE_TYPE  paUInt8
#define DATA_TYPE unsigned char
#define SAMPLE_SIZE (1)
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif