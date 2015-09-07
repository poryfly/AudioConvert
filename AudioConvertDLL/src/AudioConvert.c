#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include "AudioConvert.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"


#define AVCODEC_MAX_AUDIO_FRAME_SIZE  192000
typedef struct {
	short channels;
	short bits;
	int rate;
}head_pama;
BOOL CheckMessageQueue() 
{
	MSG msg; 
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{ 
		if(msg.message==WM_QUIT) 
			return FALSE; 
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	} 
	return TRUE; 
}
void wav_write_header(FILE* fp,head_pama pt,const int datasize)
{
    int long_temp;
    short short_temp;
    short BlockAlign;
    char *data = NULL;

    data = "RIFF";
    fwrite(data, sizeof(char), 4, fp);

    long_temp=datasize*2+36;
	fwrite(&long_temp, sizeof(long_temp), 1, fp);

    data = "WAVE";
    fwrite(data, sizeof(char), 4, fp);

    data = "fmt ";
    fwrite(data, sizeof(char), 4, fp);

    long_temp = 16;
    fwrite(&long_temp, sizeof(long_temp), 1, fp);

    short_temp = 0x0001;
    fwrite(&short_temp, sizeof(short_temp), 1, fp);

    short_temp = pt.channels;
    fwrite(&short_temp, sizeof(short_temp), 1, fp);

    long_temp = pt.rate;
    fwrite(&long_temp, sizeof(long_temp), 1, fp);
    
    long_temp = ((pt.bits)/8) * (pt.channels) * (pt.rate);
    fwrite(&long_temp, sizeof(long_temp), 1, fp);

    BlockAlign = ((pt.bits)/8) * (pt.channels);
    fwrite(&BlockAlign, sizeof(BlockAlign), 1, fp);

    short_temp = pt.bits;
    fwrite(&short_temp, sizeof(short_temp), 1, fp);

    data = "data";
    fwrite(data, sizeof(char), 4, fp);

    long_temp = datasize*(pt.channels)*(pt.bits/8);
    fwrite(&long_temp, sizeof(long_temp), 1, fp);
}

int AudioConvertFunc(const char *outfilename,int sample_rate,int channels,int sec,const char *inputfilename,HWND mParentHwnd,UINT mMsg)
{
	AVCodec *aCodec =NULL;
	AVPacket *packet = NULL;
	AVFormatContext *pFormatCtx =NULL;
    AVCodecContext *aCodecCtx= NULL;
	ReSampleContext* ResampleCtx=NULL;
	AVFrame *decoded_frame = NULL;
	int datasize;
	//int tempcount = 0;
	//long total_out_size=0;
	int64_t total_in_convert_size = 0;
	int audioConvertProgress = 0;
	int tempAudioConvertProgress;
	unsigned int i;
	int len, ret, buffer_size, count, audio_stream_index = -1, totle_samplenum = 0;

	FILE *outfile = NULL;// *infile;
	head_pama pt;

	int16_t *audio_buffer = NULL;
	int16_t *resamplebuff = NULL;
	int ResampleChange=0;
	int ChannelsChange=0;
	int tempret;

	packet = (AVPacket*)malloc(sizeof(AVPacket));
	if (packet==NULL)
	{
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)(-1));
		return -1;
	}
	packet->data=NULL;

	buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
	audio_buffer = (int16_t *)av_malloc(buffer_size);
	if (audio_buffer==NULL)
	{
		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)(-2));
		return -2;
	}
	
	av_register_all();

	av_init_packet(packet);
#if 0
	/**********尝试分解av_open_input_file函数*************/
	int ret = 0;
	
	AVFormatParameters ap = { 0 };
	AVDictionary *tmp = NULL;
	AVInputFormat *fmt = NULL;
	AVDictionary **options = NULL;
	if (!pFormatCtx && !(pFormatCtx = avformat_alloc_context()))
		return AVERROR(ENOMEM);
	if (fmt)
		pFormatCtx->iformat = fmt;

	if (options)
		av_dict_copy(&tmp, *options, 0);

	if ((ret = av_opt_set_dict(pFormatCtx, &tmp)) < 0)
		goto fail;
	
	AVDictionary *tmp = NULL;

	if (!pFormatCtx && !(pFormatCtx = avformat_alloc_context()))
		return AVERROR(ENOMEM);

	int ret;
	AVProbeData pd = {inputfilename, NULL, 0};

	if (pFormatCtx->pb) {
		pFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
		if (!pFormatCtx->iformat)
			return av_probe_input_buffer(pFormatCtx->pb, &pFormatCtx->iformat, inputfilename, pFormatCtx, 0, 0);
		else if (pFormatCtx->iformat->flags & AVFMT_NOFILE)
			av_log(pFormatCtx, AV_LOG_WARNING, "Custom AVIOContext makes no sense and "
			"will be ignored with AVFMT_NOFILE format.\n");
		return 0;
	}

	if ( (pFormatCtx->iformat && pFormatCtx->iformat->flags & AVFMT_NOFILE) ||
		(!pFormatCtx->iformat && (pFormatCtx->iformat = av_probe_input_format(&pd, 0))))
		return 0;

	URLContext *h;
	int err;

	err = ffurl_open(&h, inputfilename, AVIO_RDONLY);
	if (err < 0)
		return err;
	err = ffio_fdopen(pFormatCtx, h);
	if (err < 0) {
		ffurl_close(h);
		return err;
	}

	if (pFormatCtx->iformat)
		return 0;
	av_probe_input_buffer(pFormatCtx->pb, &pFormatCtx->iformat, inputfilename, pFormatCtx, 0, 0);


	if (pFormatCtx->iformat->flags & AVFMT_NEEDNUMBER) {
		if (!av_filename_number_test(inputfilename)) {
			ret = AVERROR(EINVAL);
			goto fail;
		}
	}

	pFormatCtx->duration = pFormatCtx->start_time = AV_NOPTS_VALUE;
	av_strlcpy(pFormatCtx->filename, inputfilename ? inputfilename : "", sizeof(pFormatCtx->filename));

	/* allocate private data */
	if (pFormatCtx->iformat->priv_data_size > 0) {
		if (!(pFormatCtx->priv_data = av_mallocz(pFormatCtx->iformat->priv_data_size))) {
			ret = AVERROR(ENOMEM);
			goto fail;
		}
		if (pFormatCtx->iformat->priv_class) {
			*(const AVClass**)pFormatCtx->priv_data = pFormatCtx->iformat->priv_class;
			av_opt_set_defaults(pFormatCtx->priv_data);
			if ((ret = av_opt_set_dict(pFormatCtx->priv_data, &tmp)) < 0)
				goto fail;
		}
	}

	/* e.g. AVFMT_NOFILE formats will not have a AVIOContext */
	if (pFormatCtx->pb)
		ff_id3v2_read(pFormatCtx, ID3v2_DEFAULT_MAGIC);

	if (!(pFormatCtx->flags&AVFMT_FLAG_PRIV_OPT) && pFormatCtx->iformat->read_header)
		if ((ret = pFormatCtx->iformat->read_header(pFormatCtx, &ap)) < 0)
			goto fail;

	if (!(pFormatCtx->flags&AVFMT_FLAG_PRIV_OPT) && pFormatCtx->pb && !pFormatCtx->data_offset)
		pFormatCtx->data_offset = avio_tell(pFormatCtx->pb);

	pFormatCtx->raw_packet_buffer_remaining_size = RAW_PACKET_BUFFER_SIZE;

	if (options) {
		av_dict_free(options);
		*options = tmp;
	}
	return 0;

fail:
	av_dict_free(&tmp);
	if (pFormatCtx->pb && !(pFormatCtx->flags & AVFMT_FLAG_CUSTOM_IO))
		avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
	pFormatCtx = NULL;
	return ret;

	return err;
	/**********尝试分解av_open_input_file函数*************/
	//pFormatCtx = avformat_alloc_context();
#endif
	ret = av_open_input_file(&pFormatCtx, inputfilename, NULL,0, NULL);

	if(ret < 0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}
		
		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)1);
		return 1;  
	}

	ret = av_find_stream_info(pFormatCtx);

	if( ret < 0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)2);
		return 2;
	}

	audio_stream_index=-1;
	for(i=0; i< (signed)pFormatCtx->nb_streams; i++)
	{

		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index < 0)
		{
			audio_stream_index = i;
			break;
		}
	}

	if(audio_stream_index == -1)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)3);
		return 3;
	}

	aCodecCtx = pFormatCtx->streams[audio_stream_index]->codec;
	if (aCodecCtx==NULL)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)4);
		return 4;
	}
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if(!aCodec) 
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		/*if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}*/
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)5);
		return 5;
	}
	//resample init
	if (channels==0)
	{
		channels=aCodecCtx->channels;
	}
	if (sample_rate==0)
	{
		sample_rate=aCodecCtx->sample_rate;
	}
	//if (aCodecCtx->channels!=channels)
	//{
	//	ChannelsChange=1;
	//	ResampleChange=1;
	//}
	if (aCodecCtx->sample_rate!=sample_rate||aCodecCtx->channels!=channels)
	{
		ResampleChange=1;
	}
	if (ResampleChange==1)
	{
		ResampleCtx = av_audio_resample_init(channels,aCodecCtx->channels,sample_rate,aCodecCtx->sample_rate,SAMPLE_FMT_S16,SAMPLE_FMT_S16,16,10,0,1.0);
		if (ResampleCtx==NULL)
		{
			if (audio_buffer!=NULL)
			{
				av_free(audio_buffer);
				audio_buffer=NULL;
			}

			if (packet->data!=NULL)
			{
				av_free_packet(packet);
				packet->data=NULL;
			}
			if (packet!=NULL)
			{
				free(packet);
				packet=NULL;
			}
			if (pFormatCtx!=NULL)
			{
				av_close_input_file(pFormatCtx);
				pFormatCtx=NULL;
			}
			/*if (aCodecCtx!=NULL)
			{
				avcodec_close(aCodecCtx);
				aCodecCtx=NULL;
			}*/
			ResampleChange=0;
			PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)6);
			return 6;
		}
		resamplebuff=(int16_t *)malloc(buffer_size);
		if (resamplebuff==NULL)
		{
			if (audio_buffer!=NULL)
			{
				av_free(audio_buffer);
				audio_buffer=NULL;
			}

			if (packet->data!=NULL)
			{
				av_free_packet(packet);
				packet->data=NULL;
			}
			if (packet!=NULL)
			{
				free(packet);
				packet=NULL;
			}
			if (pFormatCtx!=NULL)
			{
				av_close_input_file(pFormatCtx);
				pFormatCtx=NULL;
			}
			/*if (aCodecCtx!=NULL)
			{
				avcodec_close(aCodecCtx);
				aCodecCtx=NULL;
			}*/
			
			if (ResampleChange==1&&ResampleCtx!=NULL)
			{
				audio_resample_close(ResampleCtx);
				ResampleCtx=NULL;
			}
			PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)7);
			return 7;
		}
	}
	//
	datasize=sec*sample_rate;
	if(avcodec_open(aCodecCtx, aCodec)<0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}

		if (ResampleChange==1&&ResampleCtx!=NULL&&resamplebuff!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
			free(resamplebuff);
			resamplebuff=NULL;
		}
		ResampleChange=0;
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)8);
		return 8;
	}

	pt.bits = 16;
	pt.channels = channels;
	pt.rate = sample_rate;

	outfile = fopen(outfilename, "wb");
	if (!outfile) 
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}

		if (ResampleChange==1&&ResampleCtx!=NULL&&resamplebuff!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
			free(resamplebuff);
			resamplebuff=NULL;
		}
		ResampleChange=0;
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)9);
		return 9;
	}

	fseek(outfile,44,SEEK_SET);
    while(av_read_frame(pFormatCtx, packet) >= 0) 
	{
		CheckMessageQueue();
	    if(packet->stream_index == audio_stream_index)
	    {
			//while(packet->size > 0)
			//{
				buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
				len = avcodec_decode_audio3(aCodecCtx, audio_buffer, &buffer_size, packet);

				if (len < 0) 
				{
					break;
				}

				if(buffer_size > 0)
				{
					//resample
					if (ResampleChange==1)
					{
						int samples=buffer_size/ ((aCodecCtx->channels) * 2);
						int resamplenum= 0;
						resamplenum = audio_resample(ResampleCtx, 
							resamplebuff, 
							audio_buffer, 
							samples);
						count = fwrite(resamplebuff, 2*channels, resamplenum, outfile);
					}
					
					else
					{
						count = fwrite(audio_buffer, 2*aCodecCtx->channels, buffer_size/((aCodecCtx->channels)*2), outfile);
					}
					totle_samplenum += count;
				}
				//tempcount++;
				//total_out_size += count*2*aCodecCtx->channels;
				total_in_convert_size += packet->size;
				tempAudioConvertProgress = 100*total_in_convert_size/(pFormatCtx->file_size);
				if(tempAudioConvertProgress != audioConvertProgress)
				{
					if(tempAudioConvertProgress == 100)
						tempAudioConvertProgress = 99;
					audioConvertProgress = tempAudioConvertProgress;
					tempret = PostMessage(mParentHwnd,mMsg,DECING_TAG,audioConvertProgress);
				}
				if (packet->data!=NULL)
				{
					av_free_packet(packet);
					packet->data=NULL;
				}
				//packet->size -= len;
				//packet->data += len;
			//}
			if (datasize!=0&&totle_samplenum>=datasize)
			{
				break;
			}
	    }
	}
	audioConvertProgress = 100;
	PostMessage(mParentHwnd,mMsg,DECING_TAG,audioConvertProgress);
	fseek(outfile,0,SEEK_SET);
	wav_write_header(outfile, pt, totle_samplenum);
	
	if (outfile!=NULL)
	{
		fclose(outfile);
		outfile=NULL;
	}
    
	if (audio_buffer!=NULL)
	{
		av_free(audio_buffer);
		audio_buffer=NULL;
	}
	
	if (aCodecCtx!=NULL)
	{
		avcodec_close(aCodecCtx);
		aCodecCtx=NULL;
	}
	
	if (packet!=NULL)
	{
		free(packet);//
		packet=NULL;
	}
	
	if (pFormatCtx!=NULL)
	{
		av_close_input_file(pFormatCtx);
		pFormatCtx=NULL;
	}
	
	if (ResampleChange==1)
	{
		if (resamplebuff!=NULL)
		{
			free(resamplebuff);
			resamplebuff=NULL;
		}
		if (ResampleCtx!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
		}
	}
	if (totle_samplenum<=sample_rate*5)
	{
		PostMessage(mParentHwnd,mMsg,FAILED_TAG,(LPARAM)10);
		return 10;
	}
	PostMessage(mParentHwnd,mMsg,FINISH_TAG,NULL);
	return 0;
}

AUDIOCONVERT_API int AudioConvert( const char *inputfilename,const char *outfilename,int sample_rate,int channels,int sec,HWND mParentHwnd,UINT mMsg)
{
    //avcodec_register_all();
    return AudioConvertFunc(outfilename,sample_rate,channels,sec,inputfilename,mParentHwnd,mMsg);
}



int AudioConvertFunc_Buffer(const short *pcmbuffer,int sample_rate,int channels,int sec,const char *inputfilename,int *pcmbuff_size)
{
	AVCodec *aCodec =NULL;
	AVPacket *packet = NULL;
	AVFormatContext *pFormatCtx =NULL;
    AVCodecContext *aCodecCtx= NULL;
	ReSampleContext* ResampleCtx=NULL;
	AVFrame *decoded_frame = NULL;
	int datasize;

	unsigned int i;
	int len, ret, buffer_size, audio_stream_index = -1, totle_samplenum = 0;

	////1.FILE *outfile = NULL;// *infile;
	

	head_pama pt;

	int16_t *audio_buffer = NULL;
	int16_t *resamplebuff = NULL;
	int ResampleChange=0;
	int ChannelsChange=0;
	int totalsize=0;

	packet = (AVPacket*)malloc(sizeof(AVPacket));
	if (packet==NULL)
	{
		return -1;
	}
	packet->data=NULL;

	buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
	audio_buffer = (int16_t *)av_malloc(buffer_size);
	if (audio_buffer==NULL)
	{
		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		
		return -2;
	}
	
	av_register_all();

	av_init_packet(packet);

	//pFormatCtx = avformat_alloc_context();
	ret = av_open_input_file(&pFormatCtx, inputfilename, NULL,0, NULL);

	if(ret < 0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}
		
		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		
		return 1;  
	}

	ret = av_find_stream_info(pFormatCtx);

	if( ret < 0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}

		return 2;
	}

	audio_stream_index=-1;
	for(i=0; i< (signed)pFormatCtx->nb_streams; i++)
	{

		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index < 0)
		{
			audio_stream_index = i;
			break;
		}
	}

	if(audio_stream_index == -1)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}

		return 3;
	}

	aCodecCtx = pFormatCtx->streams[audio_stream_index]->codec;
	if (aCodecCtx==NULL)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		return 4;
	}
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if(!aCodec) 
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		/*if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}*/
		return 5;
	}
	//resample init
	if (channels==0)
	{
		channels=aCodecCtx->channels;
	}
	if (sample_rate==0)
	{
		sample_rate=aCodecCtx->sample_rate;
	}
	//if (aCodecCtx->channels!=channels)
	//{
	//	ChannelsChange=1;
	//	ResampleChange=1;
	//}
	if (aCodecCtx->sample_rate!=sample_rate||aCodecCtx->channels!=channels)
	{
		ResampleChange=1;
	}
	if (ResampleChange==1)
	{
		ResampleCtx = av_audio_resample_init(channels,aCodecCtx->channels,sample_rate,aCodecCtx->sample_rate,SAMPLE_FMT_S16,SAMPLE_FMT_S16,16,10,0,1.0);
		if (ResampleCtx==NULL)
		{
			if (audio_buffer!=NULL)
			{
				av_free(audio_buffer);
				audio_buffer=NULL;
			}

			if (packet->data!=NULL)
			{
				av_free_packet(packet);
				packet->data=NULL;
			}
			if (packet!=NULL)
			{
				free(packet);
				packet=NULL;
			}
			if (pFormatCtx!=NULL)
			{
				av_close_input_file(pFormatCtx);
				pFormatCtx=NULL;
			}
			/*if (aCodecCtx!=NULL)
			{
				avcodec_close(aCodecCtx);
				aCodecCtx=NULL;
			}*/
			ResampleChange=0;
			return 6;
		}
		resamplebuff=(int16_t *)malloc(buffer_size);
		if (resamplebuff==NULL)
		{
			if (audio_buffer!=NULL)
			{
				av_free(audio_buffer);
				audio_buffer=NULL;
			}

			if (packet->data!=NULL)
			{
				av_free_packet(packet);
				packet->data=NULL;
			}
			if (packet!=NULL)
			{
				free(packet);
				packet=NULL;
			}
			if (pFormatCtx!=NULL)
			{
				av_close_input_file(pFormatCtx);
				pFormatCtx=NULL;
			}
			/*if (aCodecCtx!=NULL)
			{
				avcodec_close(aCodecCtx);
				aCodecCtx=NULL;
			}*/
			
			if (ResampleChange==1&&ResampleCtx!=NULL)
			{
				audio_resample_close(ResampleCtx);
				ResampleCtx=NULL;
			}
			return 7;
		}
	}
	//
	datasize=sec*sample_rate;
	if(avcodec_open(aCodecCtx, aCodec)<0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}

		if (ResampleChange==1&&ResampleCtx!=NULL&&resamplebuff!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
			free(resamplebuff);
			resamplebuff=NULL;
		}
		ResampleChange=0;
		return 8;
	}

	pt.bits = 16;
	pt.channels = channels;
	pt.rate = sample_rate;

	////2.outfile = fopen(outfilename, "wb");
	

	////3.outfile pcmbuffer
	if (!pcmbuffer) 
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}

		if (ResampleChange==1&&ResampleCtx!=NULL&&resamplebuff!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
			free(resamplebuff);
			resamplebuff=NULL;
		}
		ResampleChange=0;
		return 9;
	}

	////4.fseek(outfile,44,SEEK_SET);


    while(av_read_frame(pFormatCtx, packet) >= 0) 
	{
		CheckMessageQueue();
	    if(packet->stream_index == audio_stream_index)
	    {
			//while(packet->size > 0)
			//{
				buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
				len = avcodec_decode_audio3(aCodecCtx, audio_buffer, &buffer_size, packet);

				if (len < 0) 
				{
					break;
				}

				if(buffer_size > 0)
				{
					//resample
					
					if (ResampleChange==1)
					{
						int samples=buffer_size/ ((aCodecCtx->channels) * 2);
						int resamplenum= 0;
						
						resamplenum = audio_resample(ResampleCtx, 
							resamplebuff, 
							audio_buffer, 
							samples);
						////5.count = fwrite(resamplebuff, 2*channels, resamplenum, outfile);
						memcpy((char*)(pcmbuffer+totalsize/2),resamplebuff,2*channels*resamplenum);
						totalsize+=2*channels*resamplenum;
						totle_samplenum+=resamplenum;
					}
					
					else
					{
						////6.count = fwrite(audio_buffer, 2*aCodecCtx->channels, buffer_size/((aCodecCtx->channels)*2), outfile);
						
						memcpy((char*)(pcmbuffer+totalsize/2),audio_buffer,2*channels*buffer_size/((aCodecCtx->channels)*2));
						totalsize+=2*channels*buffer_size/((aCodecCtx->channels)*2);
						totle_samplenum+=buffer_size/ ((aCodecCtx->channels) * 2);
						
					}
					////totle_samplenum += count;
					if(totalsize>=2*channels*(sample_rate*sec))
					{   
						*pcmbuff_size=totalsize/2/channels;
						break;
					}


				}
				if (packet->data!=NULL)
				{
					av_free_packet(packet);
					packet->data=NULL;
				}
				//packet->size -= len;
				//packet->data += len;
			//}
			if (datasize!=0&&totle_samplenum>=datasize)
			{
				break;
			}
	    }
	}
    
	if (audio_buffer!=NULL)
	{
		av_free(audio_buffer);
		audio_buffer=NULL;
	}
	
	if (aCodecCtx!=NULL)
	{
		avcodec_close(aCodecCtx);
		aCodecCtx=NULL;
	}
	
	if (packet!=NULL)
	{
		free(packet);//
		packet=NULL;
	}
	
	if (pFormatCtx!=NULL)
	{
		av_close_input_file(pFormatCtx);
		pFormatCtx=NULL;
	}
	
	if (ResampleChange==1)
	{
		if (resamplebuff!=NULL)
		{
			free(resamplebuff);
			resamplebuff=NULL;
		}
		if (ResampleCtx!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
		}
	}
	if (totle_samplenum<=sample_rate*5)
	{
		return 10;
	}
	return 0;
}

AUDIOCONVERT_API int AudioConvert_Buffer( const char *inputfilename,int sample_rate,
										int channels,int sec, short *pcmbuffer,int *pcmbuff_size)
{
	//avcodec_register_all();
	return AudioConvertFunc_Buffer(pcmbuffer,sample_rate,channels,sec,inputfilename,pcmbuff_size);
}



#if 0
int AudioConvertFunc_Buffer2(const short *pcmbuffer,int sample_rate,int channels,int sec,const char *inputfilename,int *pcmbuff_size)
{
	AVCodec *aCodec =NULL;
	AVPacket *packet = NULL;
	AVFormatContext *pFormatCtx =NULL;
    AVCodecContext *aCodecCtx= NULL;
	ReSampleContext* ResampleCtx=NULL;
	AVFrame *decoded_frame = NULL;
	int datasize;

	unsigned int i;
	int len, ret, buffer_size, audio_stream_index = -1, totle_samplenum = 0;

	////1.FILE *outfile = NULL;// *infile;
	

	head_pama pt;

	int16_t *audio_buffer = NULL;
	int16_t *resamplebuff = NULL;
	int ResampleChange=0;
	int ChannelsChange=0;
	int totalsize=0;

	packet = (AVPacket*)malloc(sizeof(AVPacket));
	if (packet==NULL)
	{
		return -1;
	}
	packet->data=NULL;

	buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
	audio_buffer = (int16_t *)av_malloc(buffer_size);
	if (audio_buffer==NULL)
	{
		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		
		return -2;
	}
	
	av_register_all();

	av_init_packet(packet);

	//pFormatCtx = avformat_alloc_context();
	ret = av_open_input_file(&pFormatCtx, inputfilename, NULL,0, NULL);

	if(ret < 0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}
		
		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		
		return 1;  
	}

	ret = av_find_stream_info(pFormatCtx);

	if( ret < 0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}

		return 2;
	}

	audio_stream_index=-1;
	for(i=0; i< (signed)pFormatCtx->nb_streams; i++)
	{

		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index < 0)
		{
			audio_stream_index = i;
			break;
		}
	}

	if(audio_stream_index == -1)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}

		return 3;
	}

	aCodecCtx = pFormatCtx->streams[audio_stream_index]->codec;
	if (aCodecCtx==NULL)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		return 4;
	}
	aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
	if(!aCodec) 
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		/*if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}*/
		return 5;
	}
	//resample init
	if (channels==0)
	{
		channels=aCodecCtx->channels;
	}
	if (sample_rate==0)
	{
		sample_rate=aCodecCtx->sample_rate;
	}
	//if (aCodecCtx->channels!=channels)
	//{
	//	ChannelsChange=1;
	//	ResampleChange=1;
	//}
	if (aCodecCtx->sample_rate!=sample_rate||aCodecCtx->channels!=channels)
	{
		ResampleChange=1;
	}
	if (ResampleChange==1)
	{
		ResampleCtx = av_audio_resample_init(channels,aCodecCtx->channels,sample_rate,aCodecCtx->sample_rate,SAMPLE_FMT_S16,SAMPLE_FMT_S16,16,10,0,1.0);
		if (ResampleCtx==NULL)
		{
			if (audio_buffer!=NULL)
			{
				av_free(audio_buffer);
				audio_buffer=NULL;
			}

			if (packet->data!=NULL)
			{
				av_free_packet(packet);
				packet->data=NULL;
			}
			if (packet!=NULL)
			{
				free(packet);
				packet=NULL;
			}
			if (pFormatCtx!=NULL)
			{
				av_close_input_file(pFormatCtx);
				pFormatCtx=NULL;
			}
			/*if (aCodecCtx!=NULL)
			{
				avcodec_close(aCodecCtx);
				aCodecCtx=NULL;
			}*/
			ResampleChange=0;
			return 6;
		}
		resamplebuff=(int16_t *)malloc(buffer_size);
		if (resamplebuff==NULL)
		{
			if (audio_buffer!=NULL)
			{
				av_free(audio_buffer);
				audio_buffer=NULL;
			}

			if (packet->data!=NULL)
			{
				av_free_packet(packet);
				packet->data=NULL;
			}
			if (packet!=NULL)
			{
				free(packet);
				packet=NULL;
			}
			if (pFormatCtx!=NULL)
			{
				av_close_input_file(pFormatCtx);
				pFormatCtx=NULL;
			}
			/*if (aCodecCtx!=NULL)
			{
				avcodec_close(aCodecCtx);
				aCodecCtx=NULL;
			}*/
			
			if (ResampleChange==1&&ResampleCtx!=NULL)
			{
				audio_resample_close(ResampleCtx);
				ResampleCtx=NULL;
			}
			return 7;
		}
	}
	//
	datasize=sec*sample_rate;
	if(avcodec_open(aCodecCtx, aCodec)<0)
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}

		if (ResampleChange==1&&ResampleCtx!=NULL&&resamplebuff!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
			free(resamplebuff);
			resamplebuff=NULL;
		}
		ResampleChange=0;
		return 8;
	}

	pt.bits = 16;
	pt.channels = channels;
	pt.rate = sample_rate;

	////2.outfile = fopen(outfilename, "wb");
	

	////3.outfile pcmbuffer
	if (!pcmbuffer) 
	{
		if (audio_buffer!=NULL)
		{
			av_free(audio_buffer);
			audio_buffer=NULL;
		}

		if (packet->data!=NULL)
		{
			av_free_packet(packet);
			packet->data=NULL;
		}
		if (packet!=NULL)
		{
			free(packet);
			packet=NULL;
		}
		if (pFormatCtx!=NULL)
		{
			av_close_input_file(pFormatCtx);
			pFormatCtx=NULL;
		}
		if (aCodecCtx!=NULL)
		{
			avcodec_close(aCodecCtx);
			aCodecCtx=NULL;
		}

		if (ResampleChange==1&&ResampleCtx!=NULL&&resamplebuff!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
			free(resamplebuff);
			resamplebuff=NULL;
		}
		ResampleChange=0;
		return 9;
	}

	////4.fseek(outfile,44,SEEK_SET);


    while(av_read_frame(pFormatCtx, packet) >= 0) 
	{
		CheckMessageQueue();
	    if(packet->stream_index == audio_stream_index)
	    {
			//while(packet->size > 0)
			//{
				buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 100;
				len = avcodec_decode_audio3(aCodecCtx, audio_buffer, &buffer_size, packet);

				if (len < 0) 
				{
					break;
				}

				if(buffer_size > 0)
				{
					//resample
					
					if (ResampleChange==1)
					{
						int samples=buffer_size/ ((aCodecCtx->channels) * 2);
						int resamplenum= 0;
						
						resamplenum = audio_resample(ResampleCtx, 
							resamplebuff, 
							audio_buffer, 
							samples);
						////5.count = fwrite(resamplebuff, 2*channels, resamplenum, outfile);
						memcpy((char*)(pcmbuffer+totalsize/2),resamplebuff,2*channels*resamplenum);
						totalsize+=2*channels*resamplenum;
						totle_samplenum+=resamplenum;
					}
					
					else
					{
						////6.count = fwrite(audio_buffer, 2*aCodecCtx->channels, buffer_size/((aCodecCtx->channels)*2), outfile);
						
						memcpy((char*)(pcmbuffer+totalsize/2),audio_buffer,2*channels*buffer_size/((aCodecCtx->channels)*2));
						totalsize+=2*channels*buffer_size/((aCodecCtx->channels)*2);
						totle_samplenum+=buffer_size/ ((aCodecCtx->channels) * 2);
						
					}
					////totle_samplenum += count;
					if(totalsize>=2*channels*(sample_rate*sec))
					{   
						*pcmbuff_size=totalsize/2/channels;
						break;
					}


				}
				if (packet->data!=NULL)
				{
					av_free_packet(packet);
					packet->data=NULL;
				}
				//packet->size -= len;
				//packet->data += len;
			//}
			if (datasize!=0&&totle_samplenum>=datasize)
			{
				break;
			}
	    }
	}
    
	if (audio_buffer!=NULL)
	{
		av_free(audio_buffer);
		audio_buffer=NULL;
	}
	
	if (aCodecCtx!=NULL)
	{
		avcodec_close(aCodecCtx);
		aCodecCtx=NULL;
	}
	
	if (packet!=NULL)
	{
		free(packet);//
		packet=NULL;
	}
	
	if (pFormatCtx!=NULL)
	{
		av_close_input_file(pFormatCtx);
		pFormatCtx=NULL;
	}
	
	if (ResampleChange==1)
	{
		if (resamplebuff!=NULL)
		{
			free(resamplebuff);
			resamplebuff=NULL;
		}
		if (ResampleCtx!=NULL)
		{
			audio_resample_close(ResampleCtx);
			ResampleCtx=NULL;
		}
	}
	if (totle_samplenum<=sample_rate*5)
	{
		return 10;
	}
	return 0;
}


int	Audio_Dec_codec(COS_HANDLE codech,COS_BYTE* buf,COS_DWORD buf_size,COS_BYTE *data,COS_DWORD *data_size,COS_SHORT *vu)
//int decode_frame(MPADecodeContext * s,
//			void *data, int *data_size,
//			uint8_t * buf, int buf_size)
{
#ifdef DEBUG_TIME
	int begin,end;
#endif
	int audio_find_header_ret;
	short cur;
	int i;
	MPADecodeContext *s = (MPADecodeContext*)codech;

    unsigned char*  pPosFound;
	s->frame_size=0;
	*data_size = 0;
	audio_find_header_ret = mp3_Inner_Find_MPEG_1_L2_Head_New(s,buf,buf_size,&pPosFound);
	
    if(mp3_find_header_ret == 0)
	{
		short *out_samples = (short*)data;
		s->inbuf_ptr = s->inbuf;    
		memcpy(s->inbuf_ptr, pPosFound, s->frame_size);
	    s->inbuf_ptr += s->frame_size;
	    buf_size -= (s->cur_offset + s->frame_size);
	    
        *data_size = mp_decode_frame(s, out_samples);
#ifdef DEBUG_TIME
	begin = sysreg_read(reg_CYCLES);
#endif	
 
#ifdef DEBUG_TIME     
	end = sysreg_read(reg_CYCLES);
    vu_count +=(end - begin);
#endif		
		s->inbuf_ptr = s->inbuf;
	   return (s->cur_offset + s->frame_size);//s->frame_size;
	}
    else if (mp3_find_header_ret == -1)
    	{
		return s->cur_offset;
    	}
	else if(mp3_find_header_ret == -2)
		return (buf_size-3);//0;	// reserve 4 bytes for the header in the next time  //0;
}
#endif