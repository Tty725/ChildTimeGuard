#include "AppConfig.h"

namespace ctg::config
{
    AppConfig MakeDefaultConfig()
    {
        AppConfig cfg;

        // 默认可用时间段：08:00 ~ 21:00
        cfg.usableTime.startMinutes = 8 * 60;
        cfg.usableTime.endMinutes = 21 * 60;

        cfg.warnMinutes = 2;             // M
        cfg.maxContinuousMinutes = 30;   // N
        cfg.restMinutes = 20;            // X

        // 默认密码 2026：UI 初始显示为 2026，配置文件仅存 Base64 编码。
        cfg.parentPinPlainUi = "2026";

        cfg.autoStart = true;

        // 语言默认自动，根据系统语言选择；配置文件中用 "auto" 表示。
        cfg.languageCode = "auto";

        return cfg;
    }
}

