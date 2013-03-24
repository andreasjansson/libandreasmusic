#include <gtest/gtest.h>
#include <iostream>
#include <exception>
#include <andreasmusic/audio.hpp>
#include <andreasmusic/exception.hpp>

using namespace andreasmusic;

#define TRY try {
#define CATCH } catch(Exception e) { std::cout << e.what() << std::endl; FAIL(); }

TEST(AudioTest, EmptyConstructor) {
  Audio audio;
  ASSERT_EQ(0, audio.get_length());
}

TEST(AudioTest, Mp3Mono)
{
  Audio audio("test_mono.mp3");
  ASSERT_EQ(1, audio.get_channels());
  ASSERT_EQ(44100, audio.get_rate());
  ASSERT_EQ(23020, audio.get_length());
}

TEST(AudioTest, Play)
{
  Audio audio("test_mono.mp3");
  audio.play();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
