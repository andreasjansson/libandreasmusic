#include <gtest/gtest.h>
#include <andreasmusic/audio.hpp>

using namespace andreasmusic;

TEST(AudioTest, EmptyConstructor) {
  Audio audio();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
