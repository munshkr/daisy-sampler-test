#define MEASURE_LOAD 1

#include <string>

#include "daisy_pod.h"
#include "fatfs_utils.h"
#include "sample_reader.h"
#include "logger.h"

#define DSY_SDRAM_BSS __attribute__((section(".sdram_bss")))

template <typename T>
T clamp(T in, T low, T high)
{
    return (in < low) ? low : (high < in) ? high : in;
}

using namespace daisy;

DaisyPod       pod;
SdmmcHandler   sdcard;
FatFSInterface fsi;
CpuLoadMeter   loadMeter;

constexpr size_t BUFSIZE      = 4096;
constexpr size_t NUM_SAMPLERS = 16; // Sample polyphony
constexpr float  SAMPLE_GAIN  = 1.0f / float(NUM_SAMPLERS);
constexpr float  MIX_VOL      = 1.0f;

int16_t      sample_buffers[NUM_SAMPLERS][BUFSIZE];
SampleReader sample_readers[NUM_SAMPLERS];


void InitMemoryCard()
{
    LOG("Initialize memory card");

    // Initialize the SDMMC Hardware
    SdmmcHandler::Config sd_cfg;
    sd_cfg.speed           = SdmmcHandler::Speed::FAST;
    sd_cfg.width           = SdmmcHandler::BusWidth::BITS_4;
    sd_cfg.clock_powersave = true;
    sdcard.Init(sd_cfg);

    // Setup our interface to the FatFS middleware
    FatFSInterface::Config fsi_config;
    fsi_config.media = FatFSInterface::Config::MEDIA_SD;
    fsi.Init(fsi_config);

    // Mount the filesystem at root
    f_mount(&fsi.GetSDFileSystem(), "/", 1);
}

void InitSampleReaders()
{
    LOG("Initialize sample readers (buffers)");

    for(size_t i = 0; i < NUM_SAMPLERS; i++)
    {
        // Clear buffer, just in case we set the buffer external RAM
        memset(sample_buffers[i], 0, BUFSIZE * sizeof(int16_t));

        sample_readers[i].Init(sample_buffers[i], BUFSIZE);
        sample_readers[i].SetLooping(false);
    }
}

void OpenAllSampleFiles()
{
    LOG("Open all samples again");

    for(size_t i = 0; i < NUM_SAMPLERS; i++)
    {
        std::string filename = std::to_string(40 + i * 2) + ".wav";
        sample_readers[i].Open(filename);
    }
}

void RestartAllSamples()
{
    LOG("Restart all samples");

    for(size_t i = 0; i < NUM_SAMPLERS; i++)
    {
        sample_readers[i].Restart();
    }
}

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
#ifdef MEASURE_LOAD
    loadMeter.OnBlockStart();
#endif

    for(size_t i = 0; i < size; i += 2)
    {
        // Mix all samplers output together
        float s = 0.0f;
        for(size_t j = 0; j < NUM_SAMPLERS; j++)
        {
            auto& reader = sample_readers[j];
            auto  samp   = reader.Process();
            TURN_LED_ON(samp == 0);
            s += samp * SAMPLE_GAIN;
        }
        out[i] = out[i + 1] = clamp(s * MIX_VOL, -1.0f, 1.0f);
    }

#ifdef MEASURE_LOAD
    loadMeter.OnBlockEnd();
#endif
}

void TimerCallback(void* data)
{
#if defined(MEASURE_LOAD)
    // Print average load (in percentage)
    const float avgLoad = loadMeter.GetAvgCpuLoad();
    const float minLoad = loadMeter.GetMinCpuLoad();
    const float maxLoad = loadMeter.GetMaxCpuLoad();
    LOG("Load: %d%% (min: %d%%, max: %d%%)",
        static_cast<int>(avgLoad * 100),
        static_cast<int>(minLoad * 100),
        static_cast<int>(maxLoad * 100));
#endif
}

void PrepareTimer()
{
    // Timer for printing CPU load
    TimerHandle         tim5;
    TimerHandle::Config tim_cfg;
    tim_cfg.periph       = TimerHandle::Config::Peripheral::TIM_5;
    tim_cfg.enable_irq   = true;
    auto tim_target_freq = 1.0f; // 1 Hz
    auto tim_base_freq   = System::GetPClk2Freq();
    tim_cfg.period       = tim_base_freq / tim_target_freq;
    tim5.Init(tim_cfg);
    tim5.SetCallback(TimerCallback);
    tim5.Start();
}

int main()
{
    pod.Init();

    pod.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    pod.SetAudioBlockSize(512);
    pod.StartAudio(AudioCallback);

    START_LOG();

    InitMemoryCard();
    InitSampleReaders();
    PrepareTimer();

    OpenAllSampleFiles();

#ifdef MEASURE_LOAD
    loadMeter.Init(pod.AudioSampleRate(), pod.AudioBlockSize());
#endif


    for(;;)
    {
        pod.ProcessDigitalControls();

        if(pod.button1.RisingEdge())
        {
            RestartAllSamples();
        }

        if(pod.button2.RisingEdge())
        {
            OpenAllSampleFiles();
        }

        // Prepare buffers for samplers as needed
        for(size_t i = 0; i < NUM_SAMPLERS; i++)
        {
            sample_readers[i].Prepare();
        }
    }
}
