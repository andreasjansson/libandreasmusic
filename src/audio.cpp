#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scope_exit.hpp>
#include <boost/format.hpp>
#include <mpg123.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <portaudio.h>
#include <utility>
#include "audio.hpp"
#include "util.hpp"
#include "exception.hpp"

namespace andreasmusic
{
  Audio::Audio()
    : filename(""),
      remote_path(""),
      channels(0),
      rate(0),
      length(0)
  {

  }

  Audio::Audio(const Audio &other)
  {
    filename = other.filename;
    remote_path = other.remote_path;
    type = other.type;
    data = other.data;
    channels = other.channels;
    rate = other.rate;
    length = other.length;
  }

  Audio::~Audio()
  {
    for(auto channel_data : data) {
      //delete channel_data;
    }

    printf("----------> filename: %s\n", filename.c_str());
    /*
      if(remote_path != "")
      remove(filename.c_str());
    */
  }

  Audio::Audio(const std::string path)
  {
    data = std::vector<AudioData>();

    if(boost::starts_with(path, "http://")) {
      filename = download(path);
      remote_path = path;
    }
    else {
      if(!boost::filesystem::exists(path))
        throw Exception(boost::str(boost::format("No such file %s") % path));
      filename = path;
      remote_path = "";
    }
    
    std::string filetype = boost::filesystem::extension(filename);
    if(filetype == ".mp3") {
      type = MP3;
      read_mp3();
    }
    else if(filetype == ".wav") {
      type = WAV;
      //read_wav();
    }
    else {
      throw Exception(boost::str(boost::format("Cannot create Audio object with filetype '%s'. Currently supported filetypes are '.wav' and '.mp3'.") % filetype));
    }
  }

  void Audio::read_mp3()
  {
    mpg123_handle *mh = NULL;

    BOOST_SCOPE_EXIT((&mh)) {
      printf("boost scope exit\n");
      if(mh) {
        printf("closing mpg123\n");
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
      }
    } BOOST_SCOPE_EXIT_END

    int err = mpg123_init();
    if(err != MPG123_OK || (mh = mpg123_new(NULL, &err)) == NULL) {
      throw Exception("Failed to initialise mpg123");
    }

    mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);
    if(mpg123_open(mh, filename.c_str()) != MPG123_OK) {
      throw Exception(boost::str(boost::format("mpg123 failed to open '%s'") % filename));
    }

    int encoding;
    if(mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
      throw Exception(boost::str(boost::format("mpg123 failed to get format from '%s'") % filename));
    }

    // ensure format doesn't change
    //mpg123_format(mh, rate, channels, encoding);
    mpg123_format_none(mh);

    off_t mp3_length = mpg123_length(mh);
    for(int i = 0; i < channels; i ++) {
      AudioData channel_data;
      if(mp3_length != MPG123_ERR) {
        channel_data.reserve(mp3_length);
      }
      data.push_back(channel_data);
    }

    int c, i;
    size_t buffer_size = mpg123_outblock(mh);
    unsigned char *buffer = (unsigned char *)
      alloca(buffer_size * sizeof(unsigned char));
    size_t done;

    length = 0;
    do {
      err = mpg123_read(mh, buffer, buffer_size, &done);

      done /= mpg123_encsize(encoding) * channels;

      for(i = 0; i < done; i ++) {
        for(c = 0; c < channels; c ++) {
          data[c].push_back(((float *)buffer)[i * channels + c]);
        }
      }
      length += done;

    } while(err == MPG123_OK);

    if(err != MPG123_DONE) {
      const char *error = err == MPG123_ERR ?
        mpg123_strerror(mh) : mpg123_plain_strerror(err);
      throw Exception(error);
    }
  }

  long Audio::get_length() const
  {
    return length;
  }

  long Audio::get_rate() const
  {
    return rate;
  }

  int Audio::get_channels() const
  {
    return channels;
  }

  AudioDataRange
  Audio::get_data_slice(long start, long end, int channel) const
  {
    if(start < 0 || end < 0 || start > length || start >= end)
      throw std::out_of_range("out of range slice");

    if(end > length)
      end = length;

    return std::make_pair(data[channel].begin() + start,
                          data[channel].begin() + end);
  }

  void Audio::play() const
  {
    Audio::Player player(this);
    player.play();
  }

  Audio::Player::Player(const Audio *audio)
  {
    this->audio = audio;
    frames_per_buffer = 64;
    pos = 0;
  }

  void Audio::Player::play()
  {
    pos = 0;
    
    PaStreamParameters output_params;
    PaStream *stream;
    PaError err;

    BOOST_SCOPE_EXIT((&stream)) {
      Pa_Terminate();
    } BOOST_SCOPE_EXIT_END

    err = Pa_Initialize();
    if(err != paNoError)
      throw Exception("Failed to initialise PortAudio");

    output_params.device = Pa_GetDefaultOutputDevice();
    if(output_params.device == paNoDevice)
      throw Exception("Failed to get PortAudio output device");

    output_params.channelCount = audio->get_channels();
    output_params.sampleFormat = paFloat32;
    output_params.suggestedLatency =
      Pa_GetDeviceInfo(output_params.device)->defaultLowOutputLatency;
    output_params.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream,
                        NULL,
                        &output_params,
                        audio->rate,
                        frames_per_buffer,
                        paClipOff,
                        audio_player_callback,
                        this);
    if(err != paNoError)
      throw Exception("Failed to open PortAudio output stream");

    err = Pa_StartStream(stream);
    if(err != paNoError)
      throw Exception("Failed to start PortAudio stream");

    Pa_Sleep(1000. * audio->get_length() / audio->get_rate());

    err = Pa_StopStream(stream);
    if(err != paNoError)
      throw Exception("Failed to stop PortAudio stream");

    err = Pa_CloseStream(stream);
    if(err != paNoError)
      throw Exception("Failed to close PortAudio stream");
  }

  int audio_player_callback(const void *input_buffer, void *output_buffer,
                            unsigned long frames_per_buffer,
                            const PaStreamCallbackTimeInfo* time_info,
                            PaStreamCallbackFlags status_flags,
                            void *user_data)
  {
    Audio::Player *player = static_cast<Audio::Player *>(user_data);
    float *out = static_cast<float *>(output_buffer);
    int channels = player->audio->get_channels();
    std::vector<AudioDataRange> slices;

    int c;
    long i;
    for(i = 0; i < channels; i ++)
      slices.push_back(player->audio->get_data_slice
                       (player->pos, player->pos + frames_per_buffer));
    
    for(i = 0; i < frames_per_buffer; i ++) {
      for(c = 0; c < channels; c ++) {
        *out++ = *(slices[c].first);
        slices[c].first ++;
        if(player->pos == player->audio->length)
          return paComplete;
      }
      player->pos ++;
    }

    return paContinue;
  }
}
