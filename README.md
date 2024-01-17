# AudioDemo

#### 介绍
音频通话示例，基于portaudio19.6.0实现录音和播放，基于socket实现网络通信，基于lame3.100实现MP3编解码

windows平台 visual studio 2015编译通过

查看本示例中能够掌握portaudio以及lame库的基本用法，学习基本的录音、播放、语音通话、MP3编解码、wav编解码（附带网络通信以及线程池）等知识



#### 文件说明
demo :可执行程序，包括网络通信

lame-3.100：LAME是遵循了GNU的MP3[编码器](https://so.csdn.net/so/search?q=编码器&spm=1001.2101.3001.7020)，编码高品质MP3的最好和唯一选择。

portaudio-19.6.0:  一个免费、跨平台、开源的音频I/O库



#### 重要说明

本示例只是个demo，未经验证，其中存在一些问题

1、在录音保存为MP3或wav格式时，有些采样格式时不支持的（具体请看AudioParameters.h文件相关注释）

2、WAV文件转MP3文件方法，经测试对于位深8位的wav文件转码效果不佳

3、MP3转WAV文件方法，是lame基础用法，能够解决大部分转码问题，但lame自带的shell命令解码调用方法不是这个流程(有兴趣可取研究源码)



