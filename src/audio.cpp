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
  }

  Audio::~Audio()
  {
    for(auto channel_data : data) {
      //delete channel_data;
    }

    if(remote_path == "")
      remove(filename.c_str());
  }

  Audio::Audio(const std::string path)
  {
    if(boost::starts_with(path, "http://")) {
      filename = download(path);
      remote_path = path;
    }
    else if(!boost::filesystem::exists(filename)) {
      throw new Exception(boost::str(boost::format("No such file %s") % filename));
      remote_path = "";
    }
    
    this->filename = filename;

    std::string filetype = boost::filesystem::extension(filename);
    if(filetype == "mp3") {
      type = MP3;
      read_mp3();
    }
    else if(filetype == "wav") {
      type = WAV;
      //read_wav();
    }
    else {
      throw new Exception(boost::str(boost::format("Cannot create Audio object with filetype '%s'. Currently supported filetypes are 'wav' and 'mp3'.") % filetype));
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

    off_t length = mpg123_length(mh);
    data = std::vector<std::vector<float> >(channels);
    for(int i = 0; i < channels; i ++) {
      if(length == MPG123_ERR)
	data[i] = std::vector<float>();
      else
	data[i] = std::vector<float>(length);
    }

    int s, c, i;
    size_t buffer_size = mpg123_outblock(mh);
    unsigned char *buffer = (unsigned char *)
      alloca(buffer_size * sizeof(unsigned char));
    size_t done;

    while((err = mpg123_read(mh, buffer, buffer_size, &done)) == MPG123_OK) {
      done /= mpg123_encsize(encoding);

      s = 0;
      for(i = 0; i < done; i ++) {
	for(c = 0; c < channels; c ++) {
	  data[c].insert(data[c].begin() + s, ((float *)buffer)[i]);
	}
	s ++;
      }
    }

    if(err != MPG123_DONE) {
      const char *error = err == MPG123_ERR ?
	mpg123_strerror(mh) : mpg123_plain_strerror(err);
      throw new Exception(error);
    }
  }
}
