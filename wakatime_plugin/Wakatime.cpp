#pragma once
#include "Plugin.h"
#include "Wakatime.h"

#include <ShlObj.h>
#include <vector>

Wakatime* g_wakatime = new Wakatime();

constexpr int kDelayBeforeSend = 120 * CLOCKS_PER_SEC;

bool is_file_exist(const std::wstring& file_name)
{
    const std::ifstream infile(file_name);
    return infile.good();
}

void GetReclassVersion(wchar_t* version)
{
    wchar_t raw_path_name[MAX_PATH];
    GetModuleFileName(nullptr, raw_path_name, MAX_PATH);

    DWORD dummy;
    const DWORD dw_size = GetFileVersionInfoSize(raw_path_name, &dummy);
    if (dw_size == 0)
    {
        ReClassPrintConsole(L"[Wakatime] GetFileVersionInfoSize failed with error %d", GetLastError());
        return;
    }

    std::vector<BYTE> data(dw_size);
    if (!GetFileVersionInfo(raw_path_name, NULL, dw_size, &data[0]))
    {
        ReClassPrintConsole(L"[Wakatime] GetFileVersionInfo failed with error %d", GetLastError());
        return;
    }

    UINT ui_ver_len = 0;
    VS_FIXEDFILEINFO* p_fixed_info = nullptr;
    if (VerQueryValue(&data[0], L"\\", reinterpret_cast<void**>(&p_fixed_info), &ui_ver_len) == 0)
    {
        ReClassPrintConsole(L"[Wakatime] Can't obtain ProductVersion from resources\n");
        return;
    }

    wsprintf(version, L"%u.%u.%u.%u",
             HIWORD(p_fixed_info->dwProductVersionMS),
             LOWORD(p_fixed_info->dwProductVersionMS),
             HIWORD(p_fixed_info->dwProductVersionLS),
             LOWORD(p_fixed_info->dwProductVersionLS));
}

Wakatime::Wakatime()
{
    wchar_t home_path[MAX_PATH + 1];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, home_path)))
    {
        MessageBoxW(nullptr, L"[Wakatime] Can't obtain home path!", L"Wakatime", 0);
    }
    else
    {
        ReClassPrintConsole(L"[Wakatime] Home path: %s", home_path);
    }

    wchar_t cli_path[MAX_PATH];
    wsprintf(cli_path, L"%s\\.wakatime\\wakatime-cli\\wakatime-cli.exe", home_path);
    ReClassPrintConsole(L"[Wakatime] CLI path: %s", cli_path);

    if (!is_file_exist(cli_path))
    {
        MessageBoxW(nullptr, L"[Wakatime] Can't find wakatime cli! Is its installed on your system?", L"Wakatime",
                    MB_ICONERROR | MB_OK);
        return;
    }

    m_cli_path_ = cli_path;

    wchar_t config_path[MAX_PATH];
    wsprintf(config_path, L"%s\\.wakatime.cfg", home_path);
    ReClassPrintConsole(L"[Wakatime] Config path: %s", config_path);

    if (!is_file_exist(config_path))
        MessageBoxW(nullptr, L"[Wakatime] Can't obtain wakatime config!", L"Wakatime", 0);
    else
        m_config_path_ = config_path;

    constexpr wchar_t wsnull[512]{};
    wchar_t buffer[1024];
    GetPrivateProfileString(
        L"settings",
        L"api_key ",
        wsnull,
        buffer,
        1024,
        config_path
    );

    if (lstrcmp(wsnull, buffer))
    {
        m_api_key_ = buffer;
        ReClassPrintConsole(L"[Wakatime] Found API Key: %s", m_api_key_.c_str());
    }
    else
    {
        MessageBoxW(nullptr, L"[Wakatime] Can't obtain wakatime api key!\nPlease set it via plugin settings.",
                    L"Wakatime",
                    MB_ICONWARNING | MB_OK);
    }

    GetReclassVersion(&m_reclass_version_.front());
}


void Wakatime::UpdateApiKey(const std::wstring& api_key)
{
    if (api_key.empty())
        return;

    m_api_key_ = api_key;

    WritePrivateProfileStringW(
        L"settings",
        L"api_key",
        m_api_key_.c_str(),
        m_config_path_.c_str()
    );
}

bool CreateCli(const wchar_t* psz_title, const wchar_t* psz_command)
{
    PROCESS_INFORMATION process_info = {};
    STARTUPINFO start_info = {};
    DWORD error_code;

    start_info.cb = sizeof(start_info);
    start_info.lpTitle = (psz_title) ? const_cast<wchar_t*>(psz_title) : const_cast<wchar_t*>(psz_command);

    start_info.wShowWindow = SW_HIDE;
    start_info.dwFlags |= STARTF_USESHOWWINDOW;

    if (CreateProcess(nullptr, const_cast<wchar_t*>(psz_command),
                      nullptr, nullptr, TRUE,
                      CREATE_NEW_PROCESS_GROUP, nullptr, nullptr,
                      &start_info, &process_info))
    {
        error_code = 0;
    }
    else
    {
        error_code = GetLastError();
    }

    if (process_info.hThread)
        CloseHandle(process_info.hThread);

    if (process_info.hProcess)
        CloseHandle(process_info.hProcess);

    return error_code == 0;
}

void Wakatime::GenerateCmdAndExecute(const bool is_write) const
{
    wchar_t command_line[2048 * 2];
    wsprintf(command_line,
             L"%s --plugin \"reclassex/%s reclassex-wakatime/%s\" --project \"%s\" --entity \"%s\" --alternate-language \"Binary\" --category \"debugging\"%s",
             m_cli_path_.c_str(),
             m_reclass_version_.c_str(),
             VERSION,
             m_project_.c_str(),
             m_entity_.c_str(),
             is_write ? L" --write" : L""
    );

#ifdef __DEBUG
    ReClassPrintConsole(command_line);
#endif
    CreateCli(m_cli_path_.c_str(), command_line);
};

void Wakatime::Heartbeat(const bool is_write)
{
    const auto time_elapsed = current_time_ - last_time_heartbeat_;

    if (m_cli_path_.empty() || m_api_key_.empty() || time_elapsed < NULL)
        return;

    if (!is_write && time_elapsed > kDelayBeforeSend)
    {
        GenerateCmdAndExecute(is_write);
        last_time_heartbeat_ = GetTickCount64();
    }
    else if (is_write)
    {
        GenerateCmdAndExecute(is_write);
        last_time_heartbeat_ = GetTickCount64();
    }

#ifdef __DEBUG
    ReClassPrintConsole(L"[Wakatime] %i", current_time_ - last_time_heartbeat_);
#endif
}
