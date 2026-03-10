#include "ShutdownController.h"

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "advapi32.lib")
#endif

namespace ctg {
namespace platform {

    namespace {

        bool EnableShutdownPrivilege()
        {
            HANDLE hToken = nullptr;
            if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                return false;

            LUID luid{};
            if (!::LookupPrivilegeValueW(nullptr, L"SeShutdownPrivilege", &luid))
            {
                ::CloseHandle(hToken);
                return false;
            }

            TOKEN_PRIVILEGES tp{};
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            BOOL ok = ::AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr);
            ::CloseHandle(hToken);
            return (ok != FALSE);
        }
    }

    bool ShutdownController::RequestShutdown()
    {
        EnableShutdownPrivilege();
        if (::ExitWindowsEx(EWX_POWEROFF | EWX_FORCE, 0) != FALSE)
            return true;
        return false;
    }

} // namespace platform
} // namespace ctg

