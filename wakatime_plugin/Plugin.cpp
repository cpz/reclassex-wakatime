#include "Plugin.h"

#include <array>

#include "resource.h"

BOOL WakaTimePluginState = FALSE;

void Timer()
{
    g_wakatime->last_time_heartbeat_ = GetTickCount64();
    while (true)
    {
        g_wakatime->current_time_ = GetTickCount64();
        Sleep(1);
    }
}

BOOL
PLUGIN_CC
PluginInit(
    OUT LPRECLASS_PLUGIN_INFO lpRCInfo
)
{
    wcscpy_s(lpRCInfo->Name, L"Wakatime");
    wcscpy_s(lpRCInfo->Version, VERSION);
    wcscpy_s(lpRCInfo->About,
             L"Wakatime integration for ReclassEx\n> https://github.com/cpz/reclassex-wakatime");
    lpRCInfo->DialogId = IDD_SETTINGS_DLG;

    if (!ReClassIsReadMemoryOverriden() && !ReClassIsWriteMemoryOverriden())
    {
        if (ReClassOverrideMemoryOperations(ReadCallback, WriteCallback) == FALSE)
        {
            ReClassPrintConsole(L"[Wakatime] Failed to register read/write callbacks, failing PluginInit");
            return FALSE;
        }
    }

    WakaTimePluginState = TRUE;
    return TRUE;
}

VOID
PLUGIN_CC
PluginStateChange(
    IN BOOL State
)
{
    //
    // Update global state variable
    //
    WakaTimePluginState = State;


    if (State)
    {
        ReClassPrintConsole(L"[WakaTime] Plugin loaded!");

        const auto h_thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Timer),
                                           nullptr,
                                           0, nullptr);
        if (h_thread)
        {
            g_wakatime->h_thread_ = h_thread;
        }

        if (!ReClassIsReadMemoryOverriden() && !ReClassIsWriteMemoryOverriden())
        {
            if (ReClassOverrideMemoryOperations(ReadCallback, WriteCallback) == FALSE)
            {
                ReClassPrintConsole(L"[WakaTime] Failed to register read/write callbacks, failing PluginInit");
            }
        }
    }
    else
    {
        ReClassPrintConsole(L"[WakaTime] Disabled!");
        TerminateThread(g_wakatime->h_thread_, EXIT_SUCCESS);

        if (ReClassGetCurrentReadMemory() == &ReadCallback)
            ReClassRemoveReadMemoryOverride();

        if (ReClassGetCurrentWriteMemory() == &WriteCallback)
            ReClassRemoveWriteMemoryOverride();
    }
}

INT_PTR
PLUGIN_CC
PluginSettingsDlg(
    IN HWND hWnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
{
    std::wstring m_api_key(4095, L'\0');

    switch (Msg)
    {
    case WM_INITDIALOG:
        {
            if (WakaTimePluginState)
                SetDlgItemText(hWnd, IDC_WAKATIME_API_KEY, g_wakatime->m_api_key_.c_str());
        }
        return TRUE;

    case WM_COMMAND:
        {
            const WORD notification_code = HIWORD(wParam);
            const WORD control_id = LOWORD(wParam);

            if (notification_code == BN_CLICKED)
            {
                if (control_id == IDC_CLOSE_UPDATE)
                {
                    m_api_key.resize(GetDlgItemText
                        (hWnd,
                         IDC_WAKATIME_API_KEY,
                         &m_api_key.front(),
                         static_cast<int>(m_api_key.size() + 1)
                        ));

                    g_wakatime->UpdateApiKey(m_api_key);
                    ReClassPrintConsole(L"[WakaTime] Updated API Key: %s", g_wakatime->m_api_key_.c_str());
                    ReClassPrintConsole(L"[WakaTime] Recieved update api key event.");
                    EndDialog(hWnd, LOWORD(wParam));
                }
            }
        }
        break;

    case WM_CLOSE:
        {
            EndDialog(hWnd, 0);
        }
        break;
    }
    return FALSE;
}

BOOL
PLUGIN_CC
WriteCallback(
    IN LPVOID Address,
    IN LPVOID Buffer,
    IN SIZE_T Size,
    OUT PSIZE_T BytesWritten
)
{
    DWORD old_protect;
    const HANDLE process_handle = ReClassGetProcessHandle();

    wchar_t process_path[MAX_PATH];
    if (GetModuleFileNameEx(process_handle, nullptr, process_path, MAX_PATH))
    {
        const auto current_project = PathFindFileName(process_path);
        g_wakatime->m_project_ = current_project;
        g_wakatime->m_entity_ = process_path;
    }

    g_wakatime->Heartbeat(true);
    VirtualProtectEx(process_handle, (PVOID)Address, Size, PAGE_EXECUTE_READWRITE, &old_protect);
    const BOOL retval = WriteProcessMemory(process_handle, (PVOID)Address, Buffer, Size, BytesWritten);
    VirtualProtectEx(process_handle, (PVOID)Address, Size, old_protect, nullptr);

    return retval;
}

BOOL
PLUGIN_CC
ReadCallback(
    IN LPVOID Address,
    IN LPVOID Buffer,
    IN SIZE_T Size,
    OUT PSIZE_T BytesRead
)
{
    const HANDLE process_handle = ReClassGetProcessHandle();

    wchar_t process_path[MAX_PATH];
    if (GetModuleFileNameEx(process_handle, nullptr, process_path, MAX_PATH))
    {
        const auto current_project = PathFindFileName(process_path);
        g_wakatime->m_project_ = current_project;
        g_wakatime->m_entity_ = process_path;
    }

    g_wakatime->Heartbeat(false);
    const BOOL retval = ReadProcessMemory(process_handle, (LPVOID)Address, Buffer, Size, BytesRead);
    if (!retval)
        ZeroMemory(Buffer, Size);
    return retval;
}
