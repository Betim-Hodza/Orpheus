// player.hpp
#pragma once
#include "miniaudio.h"
#include <string>
#include <taglib/fileref.h>

struct PlayerData
{
  ma_engine engine;
  ma_sound sound;
};

class MiniAudioPlayer
{
public:
  MiniAudioPlayer();
  ~MiniAudioPlayer();

  void init();
  void cleanup();
  bool startSong(std::string song_path);
  bool pauseSong();
  bool rewind();

private:
  PlayerData audio_state;

  bool loadSong(std::string path);
};
