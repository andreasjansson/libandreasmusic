#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <string>
#include <vector>

namespace andreasmusic
{
  enum AudioFileType { MP3, WAV };

  class Audio
  {
  public:
    Audio();
    Audio(const Audio &);
    Audio(const std::string);
    ~Audio();

    std::string filename;
    std::string remote_path;
    AudioFileType type;
    std::vector<std::vector<float> > data;
    int channels;
    long rate;

  private:
    void read_mp3();
  };
}

#endif
