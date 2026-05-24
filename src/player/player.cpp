// player.cpp
#include "player.hpp"
#include "miniaudio.h"
#include "util.hpp"

MiniAudioPlayer::MiniAudioPlayer()
{
  MiniAudioPlayer::init();
}

MiniAudioPlayer::~MiniAudioPlayer()
{
  MiniAudioPlayer::cleanup();
};

void MiniAudioPlayer::init()
{
  ma_result result;
  result = ma_engine_init(NULL, &audio_state.engine);
  if (result != MA_SUCCESS)
  {
    Util::errorPrint("init miniaudio failed! exiting...");
    return;
  }

  // make sure these are null to start
  *audio_state.sound = NULL;
  // should be MA_SUCCESS
}

void MiniAudioPlayer::cleanup()
{
  if (&audio_state.engine != NULL)
  {
    ma_engine_uninit(&audio_state.engine);
  }
  else if (&audio_state.sound != NULL)
  {
    ma_sound_uninit(audio_state.sound);
  }
}

bool MiniAudioPlayer::loadSong(std::string path)
{
  ma_result result;
  result = ma_sound_init_from_file(&audio_state.engine, path.c_str(), 0, NULL, NULL, &audio_state.sound);
  if (result != MA_SUCCESS)
  {
    return false;
  }

  return true;
}

/* *
 * @brief load song from path and starts playing it
 * @param song_path on disk
 * */
bool MiniAudioPlayer::startSong(std::string song_path)
{
  ma_result result;
  if (!loadSong(song_path))
  {
    Util::errorPrint("Couldn't load song :(");
    return false;
  }
  Util::debugPrint("Loaded song successfully!");

  result = ma_sound_start(&audio_state.sound);
  if (result != MA_SUCCESS)
  {
    Util::debugPrint("Couldn't play song :( ?");
    return false;
  }

  return true;
}

/* *
 * @brief pause song (keeps it's position of progress in the track)
 * */
bool MiniAudioPlayer::pauseSong()
{
  ma_bool32 isPlaying = ma_sound_is_playing(&audio_state.sound);
  ma_result result;

  if (isPlaying == MA_TRUE)
  {
    // pause
    result = ma_sound_stop(&audio_state.sound);
    if (result != MA_SUCCESS)
    {
      Util::debugPrint("Couldn't pause song :( ?");
      return false;
    }
  }
  else if (isPlaying == MA_FALSE)
  {
    // resume
    result = ma_sound_start(&audio_state.sound);
    if (result != MA_SUCCESS)
    {
      Util::debugPrint("Couldn't resume song :( ?");
      return false;
    }
  }

  return false;
}

/* *
 * @brief rewinds to start of currently playing song
 * */
bool MiniAudioPlayer::rewind()
{
  if (&audio_state.sound == NULL)
  {
    Util::errorPrint("No song loaded");
    return false;
  }

  ma_result result;
  result = ma_sound_seek_to_pcm_frame(&audio_state.sound, 0);

  if (result != MA_SUCCESS)
  {
    Util::debugPrint("Couldn't seek back to beginning to song ?");
    return false;
  }

  return true;
}
