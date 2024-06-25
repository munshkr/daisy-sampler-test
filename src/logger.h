#pragma once

#if !defined(TEST) && defined(LOGGER)

#include "daisy_seed.h"
#include "daisy_pod.h"

extern daisy::DaisyPod pod;

using Log = daisy::Logger<daisy::LOGGER_INTERNAL>;

#define START_LOG()      \
    Log::StartLog(true); \
    daisy::System::Delay(1000);
#define LOG(format, ...) Log::PrintLine(format, ##__VA_ARGS__);
#define LOG_(format, ...) Log::Print(format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) Log::PrintLine("ERROR: " format, ##__VA_ARGS__);
#define TURN_LED_ON(value) pod.seed.SetLed(value);

#else

#define START_LOG() ((void)0)
#define LOG(format, ...) ((void)0)
#define LOG_(format, ...) ((void)0)
#define LOG_ERROR(format, ...) ((void)0)
#define TURN_LED_ON(value) ((void)0)

#endif
