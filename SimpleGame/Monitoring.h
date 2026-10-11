#pragma once
#include "BackgroundWriter.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <chrono>
#include <stdexcept>

namespace Monitoring
{
inline std::string Quote(const std::wstring &value)
{
    int size = WideCharToMultiByte(
        CP_UTF8, 0, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(size, '\0');
    if (size > 0)
        WideCharToMultiByte(
            CP_UTF8, 0, value.data(), int(value.size()), &utf8[0], size, nullptr, nullptr);
    std::string out = "\"";
    for (unsigned char c : utf8)
    {
        if (c == '"' || c == '\\')
            out += '\\';
        if (c >= 32)
            out += char(c);
    }
    return out + "\"";
}

inline BackgroundWriter &Writer()
{
    static BackgroundWriter writer;
    return writer;
}

inline bool Due()
{
    static auto last = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    auto now = std::chrono::steady_clock::now();
    if (now - last < std::chrono::seconds(1))
        return false;
    last = now;
    return true;
}

inline void Publish(std::string json)
{
    static const std::wstring path = []
    {
        wchar_t executable[32768] = {};
        GetModuleFileNameW(nullptr, executable, 32768);
        std::wstring full(executable);
        return full.substr(0, full.find_last_of(L"\\/") + 1) + L"monitoring-state.json";
    }();
    Writer().Submit(
        [json = std::move(json)]
        {
            std::wstring temporary = path + L".tmp";
            std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
            file << json;
            file.flush();
            bool ok = file.good();
            file.close();
            if (!ok || !MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
                throw std::runtime_error("Monitoring snapshot write failed");
        });
}
}
