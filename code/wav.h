#if !defined(WAV_H)
#define VERSION "3.2.4"

#include <stdint.h>
#if !defined(WAV_IMGUI)
#include "imgui.h"
#define WAV_IMGUI
#endif

#define internal static
#define local_persist static
#define global_variable static

#if defined(_WIN32)
#define BASE_DPI 96.f
#else
#define BASE_DPI 72.f
#endif

#if WAV_SLOW
#if defined(_WIN32)
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;} // Trying to write to 0 will cause the program to crash
#elif defined(__APPLE__)
#include <Foundation/Foundation.h>
#define Assert(Expression) NSCAssert(Expression)
#endif
#else
#define Assert(Expression)
#endif


// NOTE(liam): WAV Format RIFF CODES are big-endian encoded chars (while the rest of the WAV header is little-endian, so annoying), 
// this ugly thing is necessary to convert chars to big-endian binary
#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

// NOTE(liam): using this instead of bool is to prevent compile-time type coercion
typedef int32 bool32;

enum
{
    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E')
};

typedef struct loaded_sound
{
    uint32 SampleRate;
    uint64 SampleCount;
    uint32 ChannelCount;
    void *Samples;
} loaded_sound;

typedef struct read_file_result 
{
    uint32 ContentsSize;
    void *Contents;

} read_file_result;

typedef struct WAVE_header
{
    uint32 RIFFID;
    uint32 Size;
    uint32 WAVEID;
} WAVE_header;

typedef struct WAVE_chunk
{
    uint32 ID;
    uint32 Size;
} WAVE_chunk;

typedef struct WAVE_fmt
{
    uint16 AudioFormat;
    uint16 NumChannels;
    uint32 SampleRate;
    uint32 ByteRate;
    uint16 BlockAlign;
    uint16 BitsPerSample;
} WAVE_fmt;

typedef struct riff_iterator
{
    uint8 *At;
    uint8 *Stop;
} riff_iterator;


#define SHOW_NONE               0
#define SHOW_DEMO               1
#define SHOW_SAVED_MESSAGE      2
#define SHOW_TRIM_SAVED_MESSAGE 4

#define EXPAND_NONE           0
#define EXPAND_TRIM           1
#define EXPAND_TRIM_INPUTS    2
#define EXPAND_CONVERT        4
#define EXPAND_CONVERT_SAVE   8
#define EXPAND_CHECK          16
#define EXPAND_CHECK_2        32
#define EXPAND_OUTRO          64

typedef struct file_filter
{
    const wchar_t *Filter;
    const wchar_t *Description;
} file_filter;

typedef struct ui_state
{
    uint8 ShowFlags;
    uint8 ExpandFlags;
    
    bool AddOutro;
    bool IntroAsOutro;
    
    int64 ShowSavedStart;
    int64 ShowTrimSavedStart;
    
    
    float ClearColorF[4];
    ImVec4 ClearColor;
    ImVec2 ButtonSize;
    ImVec2 MainPanelSize;

    char TrimStartBuf[64];
    char TrimEndBuf[64];
    
    // OS specific handles
    void *WindowHandle;
    void *NSWindowHandle;
    void *Device;
    void *RenderPassDescriptor;
    void *CommandQueue;

    float DpiScale;

    // File paths
    char *IntroFilePath;
    char *PodcastFilePath;
    char *OutroFilePath;
    char *SaveFilePath;
    char *TrimFilePath;
    char *TrimSavePath;
    char *VideoFilePath;
    char *VideoSavePath;
    
    
} ui_state;

// Cross platform function define macros
#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(char *Filename)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *Filename, uint32 MemorySize, void *Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_FREE(name) void name(void *Memory)
typedef PLATFORM_FREE(platform_free);

#define PLATFORM_ALLOC(name) void* name(uint32 Bytes)
typedef PLATFORM_ALLOC(platform_alloc);


#define PLATFORM_CREATE_WINDOW(name) void* name()
typedef PLATFORM_CREATE_WINDOW(platform_create_window);

#define PLATFORM_IMGUI_INIT_BACKEND(name) void name(void* WindowHandle)
typedef PLATFORM_IMGUI_INIT_BACKEND(platform_imgui_init_backend);

#define PLATFORM_IMGUI_START_FRAME(name) void name()
typedef PLATFORM_IMGUI_START_FRAME(platform_imgui_start_frame);

#define PLATFORM_IMGUI_RENDER(name) void name(ui_state *State)
typedef PLATFORM_IMGUI_RENDER(platform_imgui_render);

#define PLATFORM_IMGUI_SHUTDOWN(name) void name()
typedef PLATFORM_IMGUI_SHUTDOWN(platform_imgui_shutdown);

#define PLATFORM_SHOW_FILE_OPEN_DIALOG(name) char* name(ui_state *State, file_filter *Filter)
typedef PLATFORM_SHOW_FILE_OPEN_DIALOG(platform_show_file_open_dialog);

#define PLATFORM_SHOW_FILE_SAVE_DIALOG(name) char* name(ui_state *State, file_filter *Filter, wchar_t *SaveExtension)
typedef PLATFORM_SHOW_FILE_SAVE_DIALOG(platform_show_file_save_dialog);

#define PLATFORM_CONVERT_VIDEO(name) bool32 name(char *InputFilepath, char *OutputFilePath)
typedef PLATFORM_CONVERT_VIDEO(platform_convert_video);

#define PLATFORM_OPEN_FILE_MANAGER(name) void name(char *Filepath, ui_state *State)
typedef PLATFORM_OPEN_FILE_MANAGER(platform_open_file_manager);

#define PLATFORM_RESAMPLE_48K(name) bool32 name(char *Filepath)
typedef PLATFORM_RESAMPLE_48K(platform_resample_48k);


inline uint32 SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}


// external Forward Declarations
extern void* TrimLeadingSilence(loaded_sound *Sound);
extern bool32 Process(char *introFile, char *podcastFile, char *outroFile, char *outputFile);

// Forward Decs
bool32 Trim(loaded_sound *Sound, float TrimStartSeconds, float TrimEndSeconds, char *OutputFilePath);
extern loaded_sound LoadWAV(char *Filename);

extern PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile);
extern PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile);
extern PLATFORM_ALLOC(PlatformAlloc);
extern PLATFORM_FREE(PlatformFree);
extern PLATFORM_CREATE_WINDOW(PlatformCreateWindow);
extern PLATFORM_IMGUI_INIT_BACKEND(PlatformImguiInitBackend);
extern PLATFORM_IMGUI_START_FRAME(PlatformImguiStartFrame);
extern PLATFORM_IMGUI_RENDER(PlatformImguiRender);
extern PLATFORM_SHOW_FILE_OPEN_DIALOG(PlatformShowFileOpenDialog);
extern PLATFORM_SHOW_FILE_SAVE_DIALOG(PlatformShowFileSaveDialog);
extern PLATFORM_OPEN_FILE_MANAGER(PlatformOpenFileManager);
extern PLATFORM_IMGUI_SHUTDOWN(PlatformImguiShutdown);
extern PLATFORM_CONVERT_VIDEO(PlatformConvertVideo);
extern PLATFORM_RESAMPLE_48K(PlatformResample48k);


#define WAV_H
#endif
