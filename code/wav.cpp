#include <stdlib.h>
#include <string>
#include <cmath>
#include "wav.h"

#if defined(_WIN32)
#define LOG(...)                     \
    {                                \
        char cad[512];               \
        sprintf_s(cad, __VA_ARGS__); \
        OutputDebugString(cad);      \
    }
#else
#define LOG
#endif



internal void *
MonoToStereo(uint32 MonoSampleCount, int16 *MonoSamples)
{
    uint32 StereoSampleCount = MonoSampleCount * 2;
    uint32 StereoSampleSize = StereoSampleCount * sizeof(int16);
    void *Result = PlatformAlloc(StereoSampleSize);
    int16 *MonoSample;
    int16 *OutputSample = (int16 *)Result;
    for (
        uint32 SampleIndex = 0;
        SampleIndex < MonoSampleCount;
        ++SampleIndex)
    {
        MonoSample = MonoSamples + SampleIndex;
        *OutputSample++ = *MonoSample;
        *OutputSample++ = *MonoSample;
    }

    return Result;
}

inline riff_iterator
ParseChunkAt(void *At, void *Stop)
{
    riff_iterator Iter = {};

    Iter.At = (uint8 *)At;
    Iter.Stop = (uint8 *)Stop;

    return (Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    uint32 Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(WAVE_chunk) + Size;

    return (Iter);
}

inline bool32
IsValid(riff_iterator Iter)
{
    bool32 Result = (Iter.At < Iter.Stop);

    return (Result);
}

inline void *
GetChunkData(riff_iterator Iter)
{
    void *Result = (Iter.At + sizeof(WAVE_chunk));

    return (Result);
}

inline uint32
GetType(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    uint32 Result = Chunk->ID;

    return (Result);
}

inline uint32
GetChunkDataSize(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    uint32 Result = Chunk->Size;

    return (Result);
}

internal void *
PrependWave(uint64 Sound1SampleCount, int16 *Sound1, uint64 Sound2SampleCount, int16 *Sound2)
{
    uint64 Sound1Size = Sound1SampleCount * sizeof(int16);
    uint64 Sound2Size = Sound2SampleCount * sizeof(int16);

    void *Result = PlatformAlloc((uint32)(Sound1Size + Sound2Size));
    void *Sound1Cursor = (uint8 *)Result + Sound2Size;

    memcpy(Result, Sound2, Sound2Size);

    memcpy(Sound1Cursor, Sound1, Sound1Size);
    return (Result);
}

internal void *
AppendWave(uint64 Sound1SampleCount, int16 *Sound1, uint64 Sound2SampleCount, int16 *Sound2)
{
    uint64 Sound1Size = Sound1SampleCount * sizeof(int16);
    uint64 Sound2Size = Sound2SampleCount * sizeof(int16);

    void *Result = PlatformAlloc((uint32)(Sound1Size + Sound2Size));
    void *Sound2Cursor = (uint8 *)Result + Sound1Size;

    memcpy(Result, Sound1, Sound1Size);
    memcpy(Sound2Cursor, Sound2, Sound2Size);

    return (Result);
}

internal bool32
WriteWAV(char *Filename, uint32 MemorySize, void *Memory)
{

    WAVE_header Header = {};
    Header.RIFFID = WAVE_ChunkID_RIFF;
    Header.Size = 4 + 24 + (8 + MemorySize) + ((MemorySize % 2 == 0) ? 0 : 1);
    Header.WAVEID = WAVE_ChunkID_WAVE;

    WAVE_chunk FormatChunk = {};
    FormatChunk.ID = WAVE_ChunkID_fmt;
    FormatChunk.Size = 16;

    WAVE_fmt Format = {};
    Format.NumChannels = 2;
    Format.AudioFormat = 1;
    Format.SampleRate = 48000;
    Format.ByteRate = Format.SampleRate * sizeof(int16) * Format.NumChannels;
    Format.BlockAlign = sizeof(int16) * Format.NumChannels;
    Format.BitsPerSample = 16;

    WAVE_chunk DataChunk = {};
    DataChunk.ID = WAVE_ChunkID_data;
    DataChunk.Size = MemorySize;

    uint32 FileSize32 = sizeof(WAVE_header) + sizeof(WAVE_chunk) + sizeof(WAVE_fmt) + sizeof(WAVE_chunk) + MemorySize;
    void *Buffer = PlatformAlloc(FileSize32);
    memcpy(Buffer, &Header.RIFFID, sizeof(Header.RIFFID));

    void *Cursor = (uint8 *)Buffer + sizeof(Header.RIFFID);
    memcpy(Cursor, &Header.Size, sizeof(Header.Size));

    Cursor = (uint8 *)Cursor + sizeof(Header.Size);
    memcpy(Cursor, &Header.WAVEID, sizeof(Header.WAVEID));

    Cursor = (uint8 *)Cursor + sizeof(Header.WAVEID);
    memcpy(Cursor, &FormatChunk, sizeof(WAVE_chunk));

    Cursor = (uint8 *)Cursor + sizeof(WAVE_chunk);
    memcpy(Cursor, &Format, sizeof(WAVE_fmt));

    Cursor = (uint8 *)Cursor + sizeof(WAVE_fmt);
    memcpy(Cursor, &DataChunk, sizeof(WAVE_chunk));

    Cursor = (uint8 *)Cursor + sizeof(WAVE_chunk);
    memcpy(Cursor, Memory, MemorySize);

    if (!PlatformWriteEntireFile(Filename, FileSize32, Buffer))
    {
        Assert(!"PlatformWriteEntireFile failed.");
        return false;
    }

    PlatformFree(Buffer);
    return true;
}

loaded_sound
LoadWAV(char *Filename)
{
    loaded_sound Result = {};

    read_file_result ReadResult = PlatformReadEntireFile(Filename);
    if (ReadResult.ContentsSize != 0)
    {
        WAVE_header *Header = (WAVE_header *)ReadResult.Contents;
        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

        uint32 ChannelCount = 0;
        uint32 SampleDataSize = 0;
        uint32 SampleRate = 0;
        void *SampleData = 0;
        for (
            riff_iterator Iter = ParseChunkAt(Header + 1, (uint8 *)(Header + 1) + Header->Size - 1);
            IsValid(Iter);
            Iter = NextChunk(Iter))
        {
            switch (GetType(Iter))
            {
            case WAVE_ChunkID_fmt:
            {
                WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iter);
                if (fmt->AudioFormat != 1 ||
                    fmt->SampleRate != 48000 ||
                    fmt->BitsPerSample != 16 ||
                    (fmt->NumChannels != 1 && fmt->NumChannels != 2))
                    {
                        if (PlatformResample48k(Filename))
                        {
                            return LoadWAV(Filename);
                        }
                        else
                        {
                            Assert(!"WAV file is not 48kz PCM 16bit and Resampling failed");
                            return Result;
                        }
                    }
                Assert(fmt->BlockAlign == (sizeof(int16) * fmt->NumChannels));
                ChannelCount = fmt->NumChannels;
                SampleRate = fmt->SampleRate;
            }
            break;

            case WAVE_ChunkID_data:
            {
                SampleData = GetChunkData(Iter);
                SampleDataSize = GetChunkDataSize(Iter);
            }
            break;
            }
        }

        Assert(ChannelCount && SampleData);

        if (ChannelCount == 1)
        {
            int16 *MonoSamples = (int16 *)SampleData;
            SampleData = MonoToStereo(SampleDataSize / sizeof(int16), MonoSamples);
            PlatformFree(MonoSamples);
            ChannelCount = 2;
            SampleDataSize = SampleDataSize * 2;
        }
        
        // If we don't do this here we lose the pointer to the file memory and can't free it so this copy is unfortunate but necessary
        void *Samples = PlatformAlloc(SampleDataSize);
        memcpy(Samples, SampleData, SampleDataSize);
        
        Result.ChannelCount = ChannelCount;
        Result.SampleCount = SampleDataSize / sizeof(int16);
        Result.SampleRate = SampleRate;
        Result.Samples = Samples;
        
        PlatformFree(ReadResult.Contents);
    }

    return (Result);
}

inline double RootMeanSquare(int16 *Array, uint64 N)
{
    double Sum = 0;
    for (uint64 i = 0; i < N; ++i)
    {
        int16 Element = Array[i];
        Sum += pow(Element, 2);
    }

    return sqrt(Sum / (double)N);
}

void *
TrimLeadingSilence(loaded_sound *Sound)
{
    int16 *Sample = (int16 *)Sound->Samples;
    uint16 KernelWidth = 4800 * 2; // NOTE(liam): 1/100th of a second
    uint64 TrimWidth = 0;

    for (uint32 i = 0;
         i < Sound->SampleCount;
         i += KernelWidth)
    {
        if (i + KernelWidth > Sound->SampleCount)
            break;
        int16 *T1 = Sample + i;
        double LocalRMS = RootMeanSquare(T1, KernelWidth);
        if (LocalRMS < 5.0)
        {
            TrimWidth += KernelWidth;
        }
        else
        {
            break;
        }
    }

    if (TrimWidth == 0)
        return (void *)0;

    uint64 NewSize = (Sound->SampleCount - TrimWidth) * sizeof(int16);
    void *Trimmed = PlatformAlloc((uint32)NewSize);
    if (Trimmed)
    {
        int16 *TrimStart = Sample + TrimWidth;
        memcpy(Trimmed, TrimStart, NewSize);
        PlatformFree(Sound->Samples);
        Sound->Samples = Trimmed;
        Sound->SampleCount = NewSize / sizeof(int16);
    }
    else
    {
        Assert(!"Failed to allocate memory");
    }

    return Trimmed;
}

bool32
Trim(loaded_sound *Sound, float TrimStartSeconds, float TrimEndSeconds, char *OutputFilePath)
{
    bool32 Result = 0;
    if (TrimStartSeconds == 0.f && TrimEndSeconds == 0.f)
    {
        Result = WriteWAV(OutputFilePath, (uint32)Sound->SampleCount * sizeof(int16), Sound->Samples);
        return Result;
    }

    uint64 StartTrimWidth = ((uint64)roundf(Sound->SampleRate * TrimStartSeconds) * Sound->ChannelCount);
    uint64 EndTrimWidth = (TrimEndSeconds == 0.f) ? 0 : Sound->SampleCount - ((uint64)roundf(Sound->SampleRate * TrimEndSeconds) * Sound->ChannelCount);
    
    uint64 TrimmedSampleCount = Sound->SampleCount - StartTrimWidth - EndTrimWidth;
    uint32 TrimmedSize = (uint32)TrimmedSampleCount * sizeof(int16);
    void *Trimmed = PlatformAlloc(TrimmedSize);
    if (Trimmed)
    {
        int16 *StartSample = ((int16 *)Sound->Samples) + StartTrimWidth;
        memcpy(Trimmed, StartSample, TrimmedSize);
        Result = WriteWAV(OutputFilePath, TrimmedSize, Trimmed);
        PlatformFree(Trimmed);
    }
    else
    {
        Assert(!"Failed to allocate memory");
    }

    return Result;
}



bool32
Process(char *introFile, char *podcastFile, char *outroFile, char *outputFile)
{
    loaded_sound IntroSound = LoadWAV(introFile);
    loaded_sound PodcastSound = LoadWAV(podcastFile);

    TrimLeadingSilence(&PodcastSound);

    void *IntroComposite = PrependWave(
        PodcastSound.SampleCount,
        (int16 *)PodcastSound.Samples,
        IntroSound.SampleCount,
        (int16 *)IntroSound.Samples);

    void* AppendComposite = IntroComposite;
    uint64 Outsize = (IntroSound.SampleCount + PodcastSound.SampleCount) * sizeof(int16);
    if (outroFile)
    {
        loaded_sound OutroSound = LoadWAV(outroFile);
        AppendComposite = AppendWave(
            IntroSound.SampleCount + PodcastSound.SampleCount,
            (int16 *)IntroComposite,
            OutroSound.SampleCount,
            (int16 *)OutroSound.Samples);
        
        Outsize = Outsize + (OutroSound.SampleCount * sizeof(int16));
        PlatformFree(OutroSound.Samples);
    }

    bool32 Result = WriteWAV(outputFile, (uint32)Outsize, AppendComposite);

    PlatformFree(IntroSound.Samples);
    
    PlatformFree(PodcastSound.Samples);
    if (AppendComposite != IntroComposite)
    {
        PlatformFree(AppendComposite);
    }
    
    PlatformFree(IntroComposite);
    return Result;
}
