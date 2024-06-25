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

/** WAV Player that opens a .wav file with FatFS and provides a method of
reading it with double-buffering. */
class SampleReader
{
  public:
    SampleReader() {}
    ~SampleReader() {}

    /** Initializes the sampler buffer array and size */
    void Init(int16_t* buff, size_t buff_size);

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

    /** Collects buffer for playback when needed. */
    FRESULT Prepare();

    /** Resets the playback position to the beginning of the file immediately */
    FRESULT Restart();

    /** \return Whether the path of the open file, if any. */
    std::string GetPath() const { return path_; }

    /** Sets whether or not the current file will repeat after completing playback.
    \param loop To loop or not to loop.
    */
    inline void SetLooping(bool loop) { looping_ = loop; }

    /** \return Whether the sampler is looping or not. */
    inline bool GetLooping() const { return looping_; }

  private:
    enum BufferState
    {
        BUFFER_STATE_IDLE,
        BUFFER_STATE_PREPARE_0,
        BUFFER_STATE_PREPARE_1,
    };

    FRESULT close();

    bool playing_ = false;
    bool looping_ = false;
    bool invalid_ = false;

    std::string path_;
    size_t      data_pos_ = 0;
    FIL         fil_;
    int16_t*    buff_;
    size_t      buff_size_ = 0;
    BufferState buff_state_;
    size_t      read_ptr_ = 0;
};
