#include <string>

#include "daisy_pod.h"
#include "logger.h"

using namespace daisy;

DaisyPod pod;

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    float sig = 0;
    for(size_t i = 0; i < size; i += 2)
    {
        out[i]     = sig;
        out[i + 1] = sig;
    }
}

int main(void)
{
    // Configure and Initialize
    pod.Init();

    START_LOG();

    // blink led 3 times
    for(int i = 0; i < 6; i++)
    {
        pod.seed.SetLed(true);
        System::Delay(50);
        pod.seed.SetLed(false);
        System::Delay(50);
    }

    for(;;) {}
}
