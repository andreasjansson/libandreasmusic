#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <string>
#include <vector>
#include <portaudio.h>

namespace andreasmusic
{
  enum AudioFileType { MP3, WAV };
  enum MakeMonoMethod { USE_LEFT, USE_RIGHT, INTERLEAVE };

  typedef std::vector<float> AudioData;
  typedef std::pair<AudioData::const_iterator, AudioData::const_iterator> AudioDataRange;

  class Audio
  {
  public:
    Audio();
    Audio(const Audio &audio);
    Audio(const std::string path);
    ~Audio();

    long get_length() const;
    long get_rate() const;
    int get_channels() const;
    void resample(long rate);
    void make_mono(MakeMonoMethod method = USE_LEFT);
    AudioDataRange get_data_slice(long start, long end, int channel=0) const;
    void play() const;

  private:
    void read_mp3();

    std::string filename;
    std::string remote_path;
    AudioFileType type;
    std::vector<AudioData> data;
    int channels;
    long rate;
    long length;

    class Player
    {
    public:
      Player(const Audio *audio);
      void play();
      const Audio *audio;
      long frames_per_buffer;
      long pos;

    };

    friend
    int audio_player_callback(const void *input_buffer, void *output_buffer,
			      unsigned long frames_per_buffer,
			      const PaStreamCallbackTimeInfo* time_info,
			      PaStreamCallbackFlags status_flags,
			      void *user_data);
  };

  int audio_player_callback(const void *input_buffer, void *output_buffer,
			    unsigned long frames_per_buffer,
			    const PaStreamCallbackTimeInfo* time_info,
			    PaStreamCallbackFlags status_flags,
			    void *user_data);
}

#endif
