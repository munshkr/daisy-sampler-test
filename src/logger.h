#pragma once

#if !defined(TEST) && defined(LOGGER)

#include "daisy_seed.h"

using Log = daisy::Logger<daisy::LOGGER_INTERNAL>;

#define START_LOG()      \
    Log::StartLog(true); \
    daisy::System::Delay(1000);
#define LOG(format, ...) Log::PrintLine(format, ##__VA_ARGS__);
#define LOG_(format, ...) Log::Print(format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) Log::PrintLine("ERROR: " format, ##__VA_ARGS__);

#else

#define START_LOG() ((void)0)
#define LOG(format, ...) ((void)0)
#define LOG_(format, ...) ((void)0)
#define LOG_ERROR(format, ...) ((void)0)

#endif
