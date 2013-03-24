#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scope_exit.hpp>
#include <boost/format.hpp>
#include <mpg123.h>
#include <stdio.h>
#include <string>
#include <vector>
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

    if(remote_path != "")
      remove(filename.c_str());
  }

  Audio::Audio(const std::string path)
  {
    if(boost::starts_with(path, "http://")) {
      filename = download(path);
      remote_path = path;
    }
    else {
      if(!boost::filesystem::exists(path))
	throw new Exception(boost::str(boost::format("No such file %s") % path));
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
      throw new Exception(boost::str(boost::format("Cannot create Audio object with filetype '%s'. Currently supported filetypes are '.wav' and '.mp3'.") % filetype));
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
      throw new Exception("Failed to initialise mpg123");
    }

    mpg123_param(mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.);
    if(mpg123_open(mh, filename.c_str()) != MPG123_OK) {
      throw new Exception(boost::str(boost::format("mpg123 failed to open '%s'") % filename));
    }

    int encoding;
    if(mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
      throw new Exception(boost::str(boost::format("mpg123 failed to get format from '%s'") % filename));
    }

    // ensure format doesn't change
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, encoding);

    off_t mp3_length = mpg123_length(mh);
    data = std::vector<std::vector<float> >(channels);
    for(int i = 0; i < channels; i ++) {
      if(mp3_length == MPG123_ERR)
	data[i] = std::vector<float>();
      else
	data[i] = std::vector<float>(mp3_length);
    }

    int c, i;
    size_t buffer_size = mpg123_outblock(mh);
    unsigned char *buffer = (unsigned char *)
      alloca(buffer_size * sizeof(unsigned char));
    size_t done;

    length = 0;
    while((err = mpg123_read(mh, buffer, buffer_size, &done)) == MPG123_OK) {
      done /= mpg123_encsize(encoding);

      for(i = 0; i < done; i ++) {
	for(c = 0; c < channels; c ++) {
	  data[c].push_back(((float *)buffer)[i]);
	}
	length ++;
      }
    }

    if(err != MPG123_DONE) {
      const char *error = err == MPG123_ERR ?
	mpg123_strerror(mh) : mpg123_plain_strerror(err);
      throw new Exception(error);
    }
  }

  long Audio::get_length()
  {
    return length;
  }

  long Audio::get_rate()
  {
    return rate;
  }

  int Audio::get_channels()
  {
    return channels;
  }

  void Audio::play()
  {
    Audio::Player player(this);
    player.play();
  }

  Audio::Player::Player(const Audio *audio)
  {
    this.audio = audio;
    frames_per_buffer = 64;
  }

  void Audio::Player::play()
  {
    data_iterators = audio.get_data_iterators();

    PaStreamParameters output_params;
    PaStream *stream;
    PaError err;

    BOOST_SCOPE_EXIT() {
      Pa_Terminate();
    }

    err = Pa_Initialize();
    if(err != paNoError)
      throw new Exception("Failed to initialise PortAudio");

    output_params.device = Pa_GetDefaultOutputDevice();
    if(output_params.device == paNoDevice)
      throw new Exception("Failed to get PortAudio output device");

    output_params.channelCount = audio.channels;
    output_params.sampleFormat = paFloat32;
    output_params.suggestedLatency =
      Pa_GetDeviceInfo(output_params.device)->defaultLowOutputLatency;
    output_params.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream,
			NULL,
			&output_params,
			audio.rate,
			frames_per_buffer,
			paClipOff,
			Audio::Player::callback,
			NULL);
    if(err != paNoError)
      throw new Exception("Failed to open PortAudio output stream");

    err = Pa_StartStream(stream);
    if(err != paNoError)
      throw new Exception("Failed to start PortAudio stream");

    Pa_Sleep(audio.get_length());

    err = Pa_StopStream(stream);
    if(err != paNoError)
      throw new Exception("Failed to stop PortAudio stream");

    err = Pa_CloseStream(stream);
    if(err != paNoError)
      throw new Exception("Failed to close PortAudio stream");
  }

  int Audio::Player callback(const void *input_buffer, void *output_buffer,
                             unsigned long frames_per_buffer,
                             const PaStreamCallbackTimeInfo* time_info,
                             PaStreamCallbackFlags status_flags,
                             void *user_data)
  {
    int i, c;
    int channels = audio.get_channels;
    for(i = 0; i < frames_per_buffer, i ++) {
      for(c = 0; c < channels; c ++) {
	// TODO: a nice way to iterate through audio.data, for each
	// channel. reference here: http://www.portaudio.com/docs/v19-doxydocs/paex__sine_8c_source.html
	*out++ = *data_iterators[c];
	data_iterators[c] ++;
      }
    }
  }
}
