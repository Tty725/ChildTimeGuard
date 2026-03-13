#pragma once

#include <windows.h>

namespace ctg {
namespace platform {

    // 管理 HKCU Run 项下的开机自启动。
    class AutoStartManager
    {
    public:
        // 在当前用户下设置自启动。
        // exePath 为完整可执行文件路径（可包含引号，也可不包含）。
        // 返回 true 表示写入注册表成功。
        static bool EnableAutoStart(const wchar_t* appName, const wchar_t* exePath);

        // 取消当前用户下的自启动。
        static bool DisableAutoStart(const wchar_t* appName);

        // 查询是否当前注册表中已存在自启动项。
        static bool IsAutoStartEnabled(const wchar_t* appName);
    };

} // namespace platform
} // namespace ctg

