#pragma once

#define VERSION L"1.0"

/**
 * \brief 
 */
class Wakatime
{
public:
    Wakatime();

    void Heartbeat(bool is_write);

    void UpdateApiKey(const std::wstring& api_key);

private:
    void GenerateCmdAndExecute(bool is_write) const;

public:
    HANDLE h_thread_{};
    ULONGLONG current_time_ = 0;
    ULONGLONG last_time_heartbeat_ = 0;

    std::wstring m_entity_{};
    std::wstring m_project_{};
    std::wstring m_cli_path_{};
    std::wstring m_config_path_{};
    std::wstring m_api_key_{};
    std::wstring m_proxy_{};

private:
    std::wstring m_reclass_version_{};
};

extern Wakatime* g_wakatime;
