#define MEASURE_LOAD 1

#include <string>

#include "daisy_pod.h"
#include "fatfs_utils.h"
#include "sample_reader.h"
#include "logger.h"
#include <arm_math.h>

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

constexpr size_t NUM_SAMPLERS = 16; // Sample polyphony
constexpr float  SAMPLE_GAIN  = 1.0f / float(NUM_SAMPLERS);
constexpr float  MIX_VOL      = 1.0f;

constexpr size_t CALLBACK_BLOCK_SIZE = 32;
constexpr size_t PROCESS_BLOCK_SIZE  = 16;
static_assert(CALLBACK_BLOCK_SIZE % PROCESS_BLOCK_SIZE == 0);

static float process_buf[PROCESS_BLOCK_SIZE];

RequestManager request_manager;
SampleReader   sample_readers[NUM_SAMPLERS];

void InitMemoryCard()
{
    LOG("Initialize memory card");

    // Initialize the SDMMC Hardware
    SdmmcHandler::Config sd_cfg;
    sd_cfg.speed           = SdmmcHandler::Speed::FAST;
    sd_cfg.width           = SdmmcHandler::BusWidth::BITS_4;
    sd_cfg.clock_powersave = false;
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
        sample_readers[i].Init();
        sample_readers[i].SetRequestManager(&request_manager);
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

    size_t offset = 0;
    arm_fill_f32(0.0f, out, size);
    while(size > 0)
    {
        for(size_t j = 0; j < NUM_SAMPLERS; j++)
        {
            auto& reader = sample_readers[j];
            reader.Process(process_buf, PROCESS_BLOCK_SIZE);
            for(size_t i = 0; i < PROCESS_BLOCK_SIZE; i++)
            {
                out[offset + i * 2] = out[offset + i * 2 + 1]
                    += process_buf[i] * SAMPLE_GAIN;
            }
        }
        size -= 2 * PROCESS_BLOCK_SIZE;
        offset += 2 * PROCESS_BLOCK_SIZE;
    }

    // for(size_t i = 0; i < size; i += 2)
    // {
    //     // Mix all samplers output together
    //     float s = 0.0f;
    //     for(size_t j = 0; j < NUM_SAMPLERS; j++)
    //     {
    //         auto& reader = sample_readers[j];
    //         auto  samp   = reader.Process();
    //         // TURN_LED_ON(samp == 0);
    //         s += samp * SAMPLE_GAIN;
    //     }
    //     out[i] = out[i + 1] = clamp(s * MIX_VOL, -1.0f, 1.0f);
    // }

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

    // Print underruns of sample readers
    for(size_t i = 0; i < NUM_SAMPLERS; i++)
    {
        auto& reader = sample_readers[i];
        if(reader.underruns > 0)
        {
            LOG("SampleReader %d underrun %d times (%d total samples)",
                i,
                reader.underruns,
                reader.underrun_total_samples);
        }
        reader.underruns              = 0;
        reader.underrun_total_samples = 0;
    }
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
    pod.Init(true);

    pod.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    pod.SetAudioBlockSize(CALLBACK_BLOCK_SIZE);

    START_LOG();

    InitMemoryCard();
    InitSampleReaders();
    PrepareTimer();

    OpenAllSampleFiles();
    request_manager.HandleRequests();

    pod.StartAudio(AudioCallback);

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

        request_manager.HandleRequests();
    }
}
