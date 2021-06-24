/* MacOS Platform Layer */
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <ffmpegkit/FFmpegKit.h>

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#pragma clang diagnostic pop

#import <UniformTypeIdentifiers/UTCoreTypes.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>


#define LOG(...)              \
{                             \
    fprintf(...);             \
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

#include "wav.h"
#include "wav_gui.h"

PLATFORM_IMGUI_INIT_BACKEND(PlatformImguiInitBackend)
{
    return;
}

PLATFORM_IMGUI_START_FRAME(PlatformImguiStartFrame)
{
    return;
}

PLATFORM_IMGUI_RENDER(PlatformImguiRender)
{
    return;
}

PLATFORM_IMGUI_SHUTDOWN(PlatformImguiShutdown)
{
    return;
}

PLATFORM_ALLOC(PlatformAlloc)
{
    return malloc(Bytes);
}

PLATFORM_FREE(PlatformFree)
{
    free(Memory);
}

PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
{
    read_file_result Result = {};
    NSFileManager *FileManager;
    NSData *Buffer;
    
    FileManager = [NSFileManager defaultManager];
    NSString *nsFilename = [NSString stringWithUTF8String:Filename];
    
    if ([FileManager fileExistsAtPath: nsFilename] == YES)
    {
        Buffer = [FileManager contentsAtPath:[NSString stringWithUTF8String:Filename]];
        uint32 FileSize32 = (uint32)Buffer.length;
        Result.Contents = PlatformAlloc(FileSize32);
        NSLog(@"%s -- %p", Filename, Result.Contents);
        if (Result.Contents)
        {
            memcpy(Result.Contents, Buffer.bytes, FileSize32);
            Result.ContentsSize = FileSize32;
        }
        else
        {
            PlatformFree(Result.Contents);
            Result.Contents = 0;
        }
    }
    
    return Result;
    
}

PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    NSData *nsData = [NSData dataWithBytes:Memory length:MemorySize];
    NSFileManager *FileManager;
    
    FileManager = [NSFileManager defaultManager];
    [FileManager createFileAtPath:[NSString stringWithUTF8String:Filename] contents:nsData attributes:nil];
    return true;
}

typedef struct file_filter_m {
    NSString *Filter;
    NSString *Description;
} file_filter_m;

static file_filter_m ToFilterMac(file_filter *Filter)
{
    size_t FilterLength = wcslen(Filter->Filter);
    size_t DescLength = wcslen(Filter->Description);
    file_filter_m Result = {};
    Result.Filter = [[NSString alloc] initWithBytes:(const void*)Filter->Filter
                                             length:sizeof(wchar_t) * FilterLength
                                           encoding:NSUTF8StringEncoding];
    Result.Description = [[NSString alloc] initWithBytes:(const void*)Filter->Description
                                             length:sizeof(wchar_t) * DescLength
                                           encoding:NSUTF8StringEncoding];
    
    return Result;
}

PLATFORM_SHOW_FILE_OPEN_DIALOG(PlatformShowFileOpenDialog)
{
//    NSWindow *Window = (__bridge NSWindow *)WindowHandle;
    NSOpenPanel *Panel = [NSOpenPanel openPanel];
    [Panel setCanChooseDirectories:NO];
    [Panel setCanChooseFiles:YES];
    [Panel setAllowsMultipleSelection:NO];
    [Panel setExtensionHidden:NO];
    [Panel setAllowsOtherFileTypes:YES];
    
    if (Filter != NULL)
    {
        file_filter_m FilterM = ToFilterMac(Filter);
        if ([FilterM.Filter isEqual: @"*.wav"])
        {
            [Panel setAllowedContentTypes:[NSArray arrayWithObject:UTTypeWAV]];
        }
        
    }
    
    char *Result = NULL;
    
    if ([Panel runModal] == NSModalResponseOK) {
        
        NSURL *FileUrl = [[Panel URLs] objectAtIndex:0];
        
        NSString *FilePathNS = [[FileUrl path] stringByStandardizingPath];
        NSInteger PathLength = [FilePathNS lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        Result = (char *)PlatformAlloc((uint32)PathLength);

        const char* ConstResult = [FilePathNS UTF8String];
        strcpy(Result, ConstResult);
        
        
    }
    return Result;
}

PLATFORM_SHOW_FILE_SAVE_DIALOG(PlatformShowFileSaveDialog)
{
    NSSavePanel *Panel = [NSSavePanel savePanel];
    [Panel setCanCreateDirectories:YES];
    [Panel setShowsTagField:NO];
    [Panel setShowsToolbarButton:YES];
    [Panel setExtensionHidden:NO];
    [Panel setAllowedContentTypes:[NSArray arrayWithObject:UTTypeWAV]];
    [Panel setAllowsOtherFileTypes:NO];
      
    
    char *Result = NULL;

    if ([Panel runModal] == NSModalResponseOK)
    {
        NSURL *FileUrl = [Panel URL];
        
        NSString *FilePathNS = [[FileUrl path] stringByStandardizingPath];
        NSInteger PathLength = [FilePathNS lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
        Result = (char *)PlatformAlloc((uint32)PathLength);

        const char* ConstResult = [FilePathNS UTF8String];
        strcpy(Result, ConstResult);
    }

    return Result;
}

PLATFORM_CREATE_WINDOW(PlatformCreateWindow)
{
    return (void *)0;
}

PLATFORM_OPEN_FILE_MANAGER(PlatformOpenFileManager)
{
    NSString *FilenameNS = [NSString stringWithUTF8String:Filepath];
    NSURL *Url = [NSURL fileURLWithPath:FilenameNS];
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[Url]];
    
}

PLATFORM_CONVERT_VIDEO(PlatformConvertVideo)
{
    NSString *InputPathNS = [NSString stringWithUTF8String:InputFilepath];
    NSString *OutputPathNS = [NSString stringWithUTF8String:OutputFilePath];
    NSString *cmd = [NSString stringWithFormat:@"-y -i \"%@\" -c:a pcm_s16le -ac 2 -ar 48000 \"%@\"", InputPathNS, OutputPathNS];
    FFmpegSession *session = [FFmpegKit execute:cmd];
    ReturnCode *returnCode = [session getReturnCode];
    if ([ReturnCode isSuccess:returnCode]) {
        return 1;
    }
    
    return 0;
    
}

PLATFORM_RESAMPLE_48K(PlatformResample48k)
{
    NSString *InputPathNS = [NSString stringWithUTF8String:Filepath];
    NSString *TempPathNS = [InputPathNS stringByAppendingString:@".rtemp.wav"];
    NSString *cmd = [NSString stringWithFormat:@"-y -i \"%@\" -c:a pcm_s16le -ac 2 -ar 48000 \"%@\"", InputPathNS, TempPathNS];
    
    FFmpegSession *Session = [FFmpegKit execute:cmd];
    ReturnCode *RCode = [Session getReturnCode];
    BOOL Success = [ReturnCode isSuccess:RCode];
    if (Success)
    {
        [[NSFileManager defaultManager] removeItemAtPath:InputPathNS error:nil];
        [[NSFileManager defaultManager] copyItemAtPath:TempPathNS toPath:InputPathNS error:nil];
    }
    [[NSFileManager defaultManager] removeItemAtPath:TempPathNS error:nil];
    
    return Success;
}


int main(int, char**)
{
    static ui_state State = InitUiState();
    InitUI(&State);

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;


    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *Window = glfwCreateWindow(1280, 720, "LilyPod Audio Tool", NULL, NULL);
    if (Window == NULL) return 1;

    id <MTLDevice> device = MTLCreateSystemDefaultDevice();
    id <MTLCommandQueue> commandQueue = [device newCommandQueue];

    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplMetal_Init(device);

    NSWindow *nswin = glfwGetCocoaWindow(Window);
    CAMetalLayer *layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;

    MTLRenderPassDescriptor *renderPassDescriptor = [MTLRenderPassDescriptor new];


    
    float clear_color[4] = {1.f, 1.f, 1.f, 1.00f};

    while (!glfwWindowShouldClose(Window))
    {
        @autoreleasepool
        {
            glfwPollEvents();

            int width, height;
            glfwGetFramebufferSize(Window, &width, &height);
            layer.drawableSize = CGSizeMake(width, height);
            id<CAMetalDrawable> drawable = [layer nextDrawable];

            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
            renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
            renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            [renderEncoder pushDebugGroup:@"ImGui demo"];

            // Start the Dear ImGui frame
            ImGui_ImplMetal_NewFrame(renderPassDescriptor);
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Rendering
            RenderUi(&State);
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

            [renderEncoder popDebugGroup];
            [renderEncoder endEncoding];

            [commandBuffer presentDrawable:drawable];
            [commandBuffer commit];
        }
    }

    // Cleanup
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(Window);
    glfwTerminate();

    return 0;

}
