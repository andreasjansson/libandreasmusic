#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <string>
#include <curl/curl.h>
#include <stdio.h>
#include "exception.hpp"

namespace andreasmusic {

  std::string download(const std::string path)
  {
    char buffer[L_tmpnam];
    tmpnam(buffer);
    std::string filename = std::string(buffer);
    std::string extension = boost::filesystem::extension(path);
    filename += extension;

    FILE *fp = fopen(filename.c_str(), "wb");
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, path.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    CURLcode res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(fp);

    if(res != CURLE_OK)
      throw new Exception(boost::str(boost::format("Failed to download '%s': %s") % path % curl_easy_strerror(res)));

    return filename;
  }

}
