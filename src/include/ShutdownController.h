#pragma once

#include <windows.h>

namespace ctg {
namespace platform {

    // 统一封装关机请求逻辑。
    class ShutdownController
    {
    public:
        // 尝试执行强制关机。
        // 返回 true 表示调用成功提交；若由于组策略等原因失败，返回 false。
        static bool RequestShutdown();
    };

} // namespace platform
} // namespace ctg

