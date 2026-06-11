// player.cpp
#include "player.hpp"
#include "miniaudio.h"
#include "util.hpp"

extern "C" void miniaudio_on_song_end(void* user_data, ma_sound* pSound)
{
  (void)pSound;
  if (!user_data) return;
  static_cast<MiniAudioPlayer*>(user_data)->onSongEnd();
}

MiniAudioPlayer::MiniAudioPlayer()  = default;
MiniAudioPlayer::~MiniAudioPlayer() { cleanup(); }

void MiniAudioPlayer::init()
{
  ma_result result = ma_engine_init(NULL, &audio_state.engine);
  if (result != MA_SUCCESS)
  {
    Util::errorPrint("init miniaudio failed: " + std::string(ma_result_description(result)));
    return;
  }
}

void MiniAudioPlayer::onSongEnd()
{
  just_ended = true;
}

void MiniAudioPlayer::cleanup()
{
  if (current_index >= 0)
    stopCurrent();

  song_queue.clear();

  ma_engine_uninit(&audio_state.engine);
}

bool MiniAudioPlayer::loadSong(const std::string& path)
{
  ma_result result = ma_sound_init_from_file(&audio_state.engine, path.c_str(), 0, NULL, NULL, &audio_state.sound);
  if (result != MA_SUCCESS)
  {
    Util::errorPrint("Failed to load song: " + std::string(ma_result_description(result)));
    return false;
  }

  audio_state.sound_is_initialized = true;

  // Register end callback — audio thread sets just_ended when done
  ma_sound_set_end_callback(&audio_state.sound, miniaudio_on_song_end, this);

  // Cache length
  ma_uint64 len;
  if (ma_sound_get_length_in_pcm_frames(&audio_state.sound, &len) == MA_SUCCESS)
	{
    song_length_pcm = (int)len;
	}
  else
	{
    song_length_pcm = 0;
	}

  return true;
}

void MiniAudioPlayer::stopCurrent()
{

	if (audio_state.sound_is_initialized)
	{
		ma_sound_stop(&audio_state.sound);
		ma_sound_uninit(&audio_state.sound);
		audio_state.sound_is_initialized = false;
		current_index = -1;
	}
}

bool MiniAudioPlayer::isCurrentEnded()
{
  if (just_ended)
  {
    just_ended = false;
    return true;
  }
  return false;
}

bool MiniAudioPlayer::startSong(const std::string& song_path)
{
  stopCurrent();

  song_queue.clear();
  current_index = -1;

  if (!loadSong(song_path))
  {
    Util::errorPrint("Couldn't load song :(");
    return false;
  }
  Util::debugPrint("Loaded song successfully!");

  ma_result result = ma_sound_start(&audio_state.sound);
  if (result != MA_SUCCESS)
  {
    Util::debugPrint("Couldn't play song: " + std::string(ma_result_description(result)));
    return false;
  }

  current_index = 0;
  return true;
}

void MiniAudioPlayer::queueSong(const SongMetadata& song)
{
  song_queue.push_back(song);
}

void MiniAudioPlayer::queueSongFirst(const SongMetadata& song)
{
  song_queue.insert(song_queue.begin(), song);
}

bool MiniAudioPlayer::playCurrent()
{
  if (song_queue.empty())
	{
    return false;
	}

  current_index++;
  if (current_index < 0 || current_index >= (int)song_queue.size())
  {
    current_index--;
    return false;
  }

  const auto& cur = song_queue[current_index];
  if (!loadSong(cur.song_path))
  {
    Util::errorPrint("Couldn't load song from queue :(");
    current_index--;
    return false;
  }

  ma_result result = ma_sound_start(&audio_state.sound);
  if (result != MA_SUCCESS)
  {
    Util::debugPrint("Couldn't play song: " + std::string(ma_result_description(result)));
    return false;
  }

  return true;
}

bool MiniAudioPlayer::nextSong()
{
  if (current_index >= 0)
  {
    if (audio_state.sound_is_initialized)
    {
      ma_sound_stop(&audio_state.sound);
      ma_sound_uninit(&audio_state.sound);
      audio_state.sound_is_initialized = false;
    }
  }

  return playCurrent();
}

bool MiniAudioPlayer::prevSong()
{
  // If far enough into the song, go to next instead of restarting
  if (current_index >= 0 && ma_sound_is_playing(&audio_state.sound))
  {
    ma_uint64 cursor = 0;
    ma_uint32 sample_rate = ma_engine_get_sample_rate(&audio_state.engine);
    if (ma_sound_get_cursor_in_pcm_frames(&audio_state.sound, &cursor) == MA_SUCCESS
        && cursor >= 5 * sample_rate)
    {
      // Past 5 seconds, skip to next
      return nextSong();
    }
  }

  // Go back to previous in queue
  if (current_index >= 0)
  {
    if (audio_state.sound_is_initialized)
    {
      ma_sound_stop(&audio_state.sound);
      ma_sound_uninit(&audio_state.sound);
      memset(&audio_state.sound, 0, sizeof(audio_state.sound));
      audio_state.sound_is_initialized = false;
    }
  }

  current_index--;
  if (current_index < 0)
  {
    current_index = -1;
    return false;
  }

  return playCurrent();
}

void MiniAudioPlayer::clearQueue()
{
  if (current_index >= 0)
  {
    if (audio_state.sound_is_initialized)
    {
      ma_sound_stop(&audio_state.sound);
      ma_sound_uninit(&audio_state.sound);
      memset(&audio_state.sound, 0, sizeof(audio_state.sound));
      audio_state.sound_is_initialized = false;
    }
    current_index = -1;
  }
  song_queue.clear();
}

bool MiniAudioPlayer::pauseSong()
{
  if (current_index < 0)
	{
    return false;
	}

  ma_bool32 isPlaying = ma_sound_is_playing(&audio_state.sound);

  if (isPlaying == MA_TRUE)
	{
    return ma_sound_stop(&audio_state.sound) == MA_SUCCESS;
	}
  else
	{
    return ma_sound_start(&audio_state.sound) == MA_SUCCESS;
	}
}

bool MiniAudioPlayer::rewind()
{
  if (current_index < 0)
  {
    Util::errorPrint("No song loaded");
    return false;
  }

  return ma_sound_seek_to_pcm_frame(&audio_state.sound, 0) == MA_SUCCESS;
}

const SongMetadata* MiniAudioPlayer::getCurrentSong() const
{
  if (current_index < 0 || current_index >= (int)song_queue.size())
	{
    return nullptr;
	}

  return &song_queue[current_index];
}

std::string MiniAudioPlayer::getCurrentSongPath() const
{
  auto* song = getCurrentSong();
  if (!song) return "";
  return song->song_path;
}

const SongMetadata* MiniAudioPlayer::getQueueSong(unsigned int index) const
{
  if (index >= song_queue.size())
	{
    return nullptr;
	}

  return &song_queue[index];
}
