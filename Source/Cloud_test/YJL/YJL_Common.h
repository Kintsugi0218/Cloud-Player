// YJL_Common.h
// 公共日志类别 + 工具宏。
// 文件归属：YJL 子模块（与队友代码物理隔离）。
#pragma once

#include "CoreMinimal.h"

// 自定义日志类别：用 `LogYJL` 与队友日志区分，方便控制台过滤
DECLARE_LOG_CATEGORY_EXTERN(LogYJL, Log, All);

// 屏幕调试输出（仅 Editor / Development 构建）
#define YJL_SCREEN(Sec, Color, Fmt, ...) \
    do { \
        if (GEngine) { GEngine->AddOnScreenDebugMessage(-1, (Sec), (Color), FString::Printf(TEXT(Fmt), ##__VA_ARGS__)); } \
        UE_LOG(LogYJL, Log, TEXT(Fmt), ##__VA_ARGS__); \
    } while (0)
