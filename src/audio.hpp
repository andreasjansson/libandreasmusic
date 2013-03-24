#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <string>
#include <vector>

namespace andreasmusic
{
  enum AudioFileType { MP3, WAV };
  enum MakeMonoMethod { USE_LEFT, USE_RIGHT, INTERLEAVE };

  class Audio
  {
  public:
    Audio();
    Audio(const Audio &);
    Audio(const std::string);
    ~Audio();

    long get_length();
    long get_rate();
    int get_channels();
    void resample(long rate);
    void make_mono(MakeMonoMethod method = USE_LEFT);
    void play();

  private:
    void read_mp3();

    std::string filename;
    std::string remote_path;
    AudioFileType type;
    std::vector<std::vector<float> > data;
    int channels;
    long rate;
    long length;

    class Player
    {
      
    };
  };
}

#endif
