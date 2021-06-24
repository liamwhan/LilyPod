#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include <PathCch.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <stringapiset.h>
#include <tchar.h>
#include <WinBase.h>
#include <winuser.h>
#define LOG(...)                     \
    {                                \
        char cad[512];               \
        sprintf_s(cad, __VA_ARGS__); \
        OutputDebugString(cad);      \
    }

#include "wav.cpp"
#include "wav_gui.cpp"

#define TOCHAR(name) char *name(wchar_t *input)
TOCHAR(ToChar)
{

    if (!input)
        return ((char *)0);
    uint64 origsize = wcslen(input) + 1; // Add 1 to account for terminating NULL char
    uint64 newsize = origsize * 2;
    uint64 convertedChars = 0;

    char *Result = new char[newsize];
    wcstombs_s(&convertedChars, Result, newsize, input, _TRUNCATE);
    return Result;
}
#define TOWCHAR(name) wchar_t *name(char *input)
inline TOWCHAR(ToWideChar)
{
    if (!input)
        return (wchar_t *)0;

    uint32 RequiredSizeChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input, -1, NULL, 0);
    wchar_t *Result = (wchar_t *)PlatformAlloc(RequiredSizeChars * sizeof(wchar_t));

    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            input,
            -1,
            Result,
            RequiredSizeChars))
    {
        return Result;
    }

    uint32 ErrorCode = GetLastError();
    switch (ErrorCode)
    {
    case ERROR_INSUFFICIENT_BUFFER:
        LOG("Insufficient Buffer Size");
        break;
    case ERROR_INVALID_FLAGS:
        LOG("Invalid Flags");
        break;
    case ERROR_INVALID_PARAMETER:
        LOG("Invalid Parameter");
        break;
    case ERROR_NO_UNICODE_TRANSLATION:
        LOG("Inputs string could not be converted to unicode");
        break;
    }
    return (wchar_t *)0;
}

std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
    {
        return std::string(); //No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);

    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);

    return message;
}

internal void OutputLastError()
{
    OutputDebugString(GetLastErrorAsString().c_str());
    OutputDebugString("\n");
}

static ID3D11Device *g_pd3dDevice = NULL;
static ID3D11DeviceContext *g_pd3dDeviceContext = NULL;
static IDXGISwapChain *g_pSwapChain = NULL;
static ID3D11RenderTargetView *g_mainRenderTargetView = NULL;

// Forward declarations of helper functions defined at the bottom of this file
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

PLATFORM_FREE(PlatformFree)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

PLATFORM_ALLOC(PlatformAlloc)
{
    return (VirtualAlloc(0, Bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
}

PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
{
    read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = PlatformAlloc(FileSize32);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                    (FileSize32 == BytesRead))
                {
                    // File read successfully
                    Result.ContentsSize = FileSize32;
                    LOG("File %s read success\r\n", Filename);
                }
                else
                {
                    PlatformFree(Result.Contents);
                    Result.Contents = 0;
                    LOG("Unable to read file %s\r\n", Filename);
                }
            }
        }
        CloseHandle(FileHandle);
    }
    else
    {
        Assert(!"Invalid File Handle");
    }
    return (Result);
}

PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile)
{
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // File write successful
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            LOG("Unable to write file %s\r\n", Filename);
        }

        CloseHandle(FileHandle);
    }

    return (Result);
}

PLATFORM_IMGUI_INIT_BACKEND(PlatformImguiInitBackend)
{
    HWND hWnd = *((HWND *)WindowHandle);

    // Setup Win32 Platform with DX11 Backend
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

PLATFORM_IMGUI_START_FRAME(PlatformImguiStartFrame)
{
    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

PLATFORM_IMGUI_RENDER(PlatformImguiRender)
{
    // Backend specific Rendering
    const float clear_color_with_alpha[4] = {State->ClearColor.x * State->ClearColor.w, State->ClearColor.y * State->ClearColor.w, State->ClearColor.z * State->ClearColor.w, State->ClearColor.w};
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0); // Present with vsync, pass (0, 0) to disable vsync
}

PLATFORM_IMGUI_SHUTDOWN(PlatformImguiShutdown)
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    CleanupDeviceD3D();
}

// Convert FFMPEG
PLATFORM_CONVERT_VIDEO(PlatformConvertVideo)
{
    if (!system(NULL))
    {
        Assert(!"Cannot convert, command processor unavailable");
    }

    char InputPathQuoted[256];
    strcpy_s(InputPathQuoted, "\"");
    strcat_s(InputPathQuoted, InputFilepath);
    strcat_s(InputPathQuoted, "\"");

    char OutputPathQuoted[256];
    strcpy_s(OutputPathQuoted, "\"");
    strcat_s(OutputPathQuoted, OutputFilePath);
    strcat_s(OutputPathQuoted, "\"");

    char Command[1024];
    strcpy_s(Command, "ffmpeg -y -i ");
    strcat_s(Command, InputPathQuoted);
    strcat_s(Command, " -c:a pcm_s16le -ac 2 -ar 48000 ");
    strcat_s(Command, OutputPathQuoted);
    system(Command);

    return 1;
}
PLATFORM_RESAMPLE_48K(PlatformResample48k)
{
    if (!system(NULL))
    {
        Assert(!"Cannot resample, command processor unavailable");
    }

    char TempOutputPath[256];
    strcpy_s(TempOutputPath, Filepath);
    strcat_s(TempOutputPath, ".rtemp.wav");

    char TempOutputPathQuoted[256];
    strcpy_s(TempOutputPathQuoted, "\"");
    strcat_s(TempOutputPathQuoted, TempOutputPath);
    strcat_s(TempOutputPathQuoted, "\"");

    char InputPathQuoted[256];
    strcpy_s(InputPathQuoted, "\"");
    strcat_s(InputPathQuoted, Filepath);
    strcat_s(InputPathQuoted, "\"");

    char Command[1024];
    strcpy_s(Command, "ffmpeg -y -i ");
    strcat_s(Command, InputPathQuoted);
    strcat_s(Command, " -c:a pcm_s16le -ac 2 -ar 48000 ");
    strcat_s(Command, TempOutputPathQuoted);
    system(Command);

    CopyFile(TempOutputPath, Filepath, FALSE);
    if (!DeleteFile(TempOutputPath))
    {
        LOG("Unable to delete temp resampling file\n");
        OutputLastError();
    }

    return 1;
}

PLATFORM_SHOW_FILE_SAVE_DIALOG(PlatformShowFileSaveDialog)
{
    HWND hWnd = *((HWND *)State->WindowHandle);

    wchar_t *FilePath = 0;
    char *Result = 0;

    IFileSaveDialog *Dialog;

    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void **>(&Dialog));

    if (SUCCEEDED(hr))
    {
        COMDLG_FILTERSPEC FileTypeFilters;
        if (Filter)
        {
            FileTypeFilters = {Filter->Description, Filter->Filter};
        }
        else
        {
            FileTypeFilters = {L"WAV files", L"*.wav"};
        }
        wchar_t *Ext = (SaveExtension) ? SaveExtension : L"wav";
        Dialog->SetFileTypes(1, &FileTypeFilters);
        Dialog->SetDefaultExtension(Ext);
        hr = Dialog->Show(hWnd);
        if (SUCCEEDED(hr))
        {
            IShellItem *Item;
            hr = Dialog->GetResult(&Item);
            if (SUCCEEDED(hr))
            {
                hr = Item->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);
                if (!SUCCEEDED(hr))
                {
                    LOG("Failed to get file path (Item->GetDisplayName) from File Save Dialog");
                }
                else
                {
                    Result = ToChar(FilePath);
                }
                CoTaskMemFree(FilePath);
            }
            Item->Release();
        }
        Dialog->Release();
    }
    return Result;
}

PLATFORM_SHOW_FILE_OPEN_DIALOG(PlatformShowFileOpenDialog)
{
    HWND hWnd = *((HWND *)State->WindowHandle);

    wchar_t *FilePath = 0;
    char *Result = 0;

    IFileOpenDialog *Dialog;

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void **>(&Dialog));

    if (SUCCEEDED(hr))
    {
        COMDLG_FILTERSPEC FileTypeFilters;
        if (Filter)
        {
            FileTypeFilters = {Filter->Description, Filter->Filter};
        }
        else
        {
            FileTypeFilters = {L"WAV files", L"*.wav"};
        }

        Dialog->SetFileTypes(1, &FileTypeFilters);

        hr = Dialog->Show(hWnd);
        if (SUCCEEDED(hr))
        {

            IShellItem *Item;
            hr = Dialog->GetResult(&Item);
            if (SUCCEEDED(hr))
            {

                hr = Item->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

                if (!SUCCEEDED(hr))
                {
                    LOG("Failed to get file path (Item->GetDisplayName) from File Open Dialog");
                }
                else
                {
                    Result = ToChar(FilePath);
                }
                CoTaskMemFree(FilePath);
            }
            Item->Release();
        }
        Dialog->Release();
    }

    return Result;
}

PLATFORM_OPEN_FILE_MANAGER(PlatformOpenFileManager)
{
    HWND hwnd = *((HWND *)State->WindowHandle);
    SHELLEXECUTEINFOA ShellInfo = {};
    wchar_t *FilepathW = ToWideChar(Filepath);
    HRESULT hr = PathCchRemoveFileSpec(FilepathW, strlen(Filepath) + 1);
    if (SUCCEEDED(hr))
    {
        ShellInfo.fMask = SEE_MASK_DEFAULT | SEE_MASK_NO_CONSOLE;
        ShellInfo.lpVerb = "open";
        ShellInfo.hwnd = hwnd;
        ShellInfo.lpFile = ToChar(FilepathW);
        ShellInfo.nShow = SW_SHOWDEFAULT;
        ShellInfo.cbSize = sizeof(SHELLEXECUTEINFOA);

        ShellExecuteExA(&ShellInfo);
    }
    else
    {
        Assert(!"Remove filename from filepath failed");
    }
}



// Forward decs
LRESULT WINAPI
WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

internal char *
ShowFileOpenDialog(HWND hWnd);

internal char *
ShowFileSaveDialog(HWND hWnd);

int CALLBACK
WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowWindowFlag)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    uint32 DotsPerInch = GetDpiForSystem();
    float DpiScaleFactor =((float)DotsPerInch / BASE_DPI);
    WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("LilyPod Audio Utility"), NULL};

    ::RegisterClassEx(&wc);

    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("LilyPod Audio Utility"), WS_OVERLAPPEDWINDOW, 100 * (int)DpiScaleFactor, 100 * (int)DpiScaleFactor, 800 * (int) DpiScaleFactor, 600 * (int)DpiScaleFactor, NULL, NULL, wc.hInstance, NULL);
    
   
    
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Initialise COM for Win32 Dialogs
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (!SUCCEEDED(hr))
    {
        LOG("CoInitializeEx failed");
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    ui_state State = InitUiState();
    State.DpiScale = DpiScaleFactor;
    State.WindowHandle = &hwnd;
    State.ButtonSize = ImVec2(State.ButtonSize.x * DpiScaleFactor, State.ButtonSize.y * DpiScaleFactor);
    InitUI(&State);
    PlatformImguiInitBackend(&hwnd);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        RenderUi(&State);
    }

    // Cleanup
    ShutdownUi();

    CoUninitialize();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI
WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_CREATE:
    {
        HANDLE hIcon = LoadImage(0, _T("favicon.ico"), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
        Assert(hIcon != 0);
        SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(GetWindow(hWnd, GW_OWNER), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(GetWindow(hWnd, GW_OWNER), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        return 0;
    }
    case WM_SIZE:
    {
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    }
    case WM_SYSCOMMAND:
    {
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    }
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        return 0;
    }
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)
    {
        g_pSwapChain->Release();
        g_pSwapChain = NULL;
    }
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = NULL;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = NULL;
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D *pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = NULL;
    }
}
