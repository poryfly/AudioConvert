#ifndef AUDIOCONVERT_H
#define AUDIOCONVERT_H

#ifdef AUDIOCONVERT_EXPORTS  
#define AUDIOCONVERT_API __declspec(dllexport)  
#else  
#define AUDIOCONVERT_API __declspec(dllimport)  
#endif 

#define DECING_TAG 0
#define FINISH_TAG 1
#define FAILED_TAG 2

#ifdef __cplusplus
extern "C" AUDIOCONVERT_API int AudioConvert( const char *inputfilename,const char *outfilename,int sample_rate,int channels,int sec,HWND mParentHwnd,UINT mMsg);
#else
AUDIOCONVERT_API int AudioConvert( const char *inputfilename,const char *outfilename,int sample_rate,int channels,int sec,HWND mParentHwnd,UINT mMsg);
#endif 


#ifdef __cplusplus
extern "C" AUDIOCONVERT_API int AudioConvert_Buffer( const char *inputfilename,int sample_rate,int channels,int sec, short *pcmbuffer,int *pcmbuff_size);
#else
AUDIOCONVERT_API int AudioConvert_Buffer( const char *inputfilename,int sample_rate,int channels,int sec, short *pcmbuffer,int *pcmbuff_size);
#endif 

#endif
