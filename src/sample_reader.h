/* Current Limitations:
- 1x Playback speed only
- 16-bit, mono files only (otherwise fun weirdness can happen).
- Only 1 file playing back at a time.
- Not sure how this would interfere with trying to use the SDCard/FatFs outside
  of this module. However, by using the extern'd SDFile, etc. I think that would
  break things.
*/
#pragma once

#include <string>

#include "daisy_core.h"
#include "daisy_seed.h"
#include "ff.h"
#include "request_manager.h"

/** WAV Player that opens a .wav file with FatFS and provides a method of
reading it with double-buffering. */
class SampleReader : public Requester
{
  public:
    void Init();

    /** Opens the file for reading.
    \param path File to open
     */
    FRESULT Open(std::string path);

    /** Closes whatever file is currently open.
    \return &
     */
    FRESULT Close();

    /** \return The next sample if playing, otherwise returns 0 */
    float Process();

    void Process(float* out, size_t size);

    /** Resets the playback position to the beginning of the file immediately */
    void Restart();

    /** \return Whether the path of the open file, if any. */
    std::string GetPath() const { return path_; }

    size_t underruns = 0; // Number of times we've underrun the FIFO
    size_t underrun_total_samples = 0; // Total number of samples underrun.

  private:
    static constexpr size_t LOAD_THRESHOLD = FIFO_SIZE / 2;

    FRESULT close();
    void    requestNewSamples(size_t num_samples);
    void    requestRestart();

    bool playing_        = false;
    bool invalid_        = false;
    bool awaiting_start_ = false;

    std::string path_;
    size_t      data_pos_ = 0;
    FIL         fil_;

    // FIFO<int16_t, FIFO_SIZE> fifo_;
    RingBuffer<int16_t, FIFO_SIZE> ringbuf_;

    size_t underrun_samples_ = 0; // Number of samples we've underrun so far.
};
