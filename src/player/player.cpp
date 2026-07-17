// player.cpp
#include "player.hpp"
#include "miniaudio.h"
#include "util.hpp"

extern "C" void miniaudio_on_song_end(void *user_data, ma_sound *pSound)
{
  (void)pSound;
  if (!user_data)
    return;
  static_cast<MiniAudioPlayer *>(user_data)->onSongEnd();
}

MiniAudioPlayer::MiniAudioPlayer() = default;
MiniAudioPlayer::~MiniAudioPlayer()
{
  cleanup();
}

void MiniAudioPlayer::init()
{
  ma_result result = ma_engine_init(NULL, &audio_state.engine);
  if (result != MA_SUCCESS)
  {
    Util::errorPrint("init miniaudio failed: " + std::string(ma_result_description(result)));
    return;
  }
  // start splitter for EQ (lives for life of engine)
  if (!initVisualizerAudio())
  {
    // for now we'll exit early on fail (should succeed every time)
    Util::errorPrint("initVisualizerAudio failed");
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

bool MiniAudioPlayer::loadSong(const std::string &path)
{
  // sound plays from splitter so we pass it to not attach to the original endpoint
  ma_result result = ma_sound_init_from_file(&audio_state.engine, path.c_str(), MA_SOUND_FLAG_NO_DEFAULT_ATTACHMENT, NULL, NULL, &audio_state.sound);
  if (result != MA_SUCCESS)
  {
    Util::errorPrint("Failed to load song: " + std::string(ma_result_description(result)));
    return false;
  }
  // attach audio to speaker output
  result = ma_node_attach_output_bus(&audio_state.sound, 0, &audio_state.splitter, 0);
  if (result != MA_SUCCESS)
  {
    Util::errorPrint("Failed to attach audio to splitter 0: " + std::string(ma_result_description(result)));
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

bool MiniAudioPlayer::isAtEnd() const
{
  if (!audio_state.sound_is_initialized)
    return false;
  return ma_sound_at_end(&audio_state.sound) != 0;
}

bool MiniAudioPlayer::startSong(const std::string &song_path)
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

void MiniAudioPlayer::queueSong(const SongMetadata &song)
{
  song_queue.push_back(song);
}

void MiniAudioPlayer::queueSongFirst(const SongMetadata &song)
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

  const auto &cur = song_queue[current_index];
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
  if (current_index == -1)
  {
    // nothing is playing
    return false;
  }

  if (current_index >= 0)
  {
    if (audio_state.sound_is_initialized)
    {
      ma_sound_stop(&audio_state.sound);
      ma_sound_uninit(&audio_state.sound);
      memset(&audio_state.sound, 0, sizeof(audio_state.sound));
      audio_state.sound_is_initialized = false;
    }

    // play_Current adds to the index by 1
    // if the index is < 1, we dont keep on subtracting (playCurrent isn't called)
    current_index--;
    if (current_index < 0)
    {
      current_index = -1;
      return false;
    }
    current_index--;

    return playCurrent();
  }

  return false;
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

const SongMetadata *MiniAudioPlayer::getCurrentSong() const
{
  if (current_index < 0 || current_index >= (int)song_queue.size())
  {
    return nullptr;
  }

  return &song_queue[current_index];
}

std::string MiniAudioPlayer::getCurrentSongPath() const
{
  auto *song = getCurrentSong();
  if (!song)
    return "";
  return song->song_path;
}

const SongMetadata *MiniAudioPlayer::getQueueSong(unsigned int index) const
{
  if (index >= song_queue.size())
  {
    return nullptr;
  }

  return &song_queue[index];
}

int MiniAudioPlayer::getCurrentPositionSeconds() const
{
  if (current_index < 0 || !audio_state.sound_is_initialized)
  {
    return 0;
  }

  ma_uint64 cursor = 0;
  if (ma_sound_get_cursor_in_pcm_frames(&audio_state.sound, &cursor) != MA_SUCCESS)
  {
    return 0;
  }

  ma_uint32 sr = ma_engine_get_sample_rate(&audio_state.engine);
  if (sr == 0)
  {
    return 0;
  }

  return static_cast<int>(cursor / sr);
}

int MiniAudioPlayer::getSongLengthSeconds() const
{
  if (song_length_pcm <= 0)
  {
    return 0;
  }

  ma_uint32 sr = ma_engine_get_sample_rate(&audio_state.engine);
  if (sr == 0)
  {
    return 0;
  }

  return static_cast<int>(song_length_pcm / sr);
}

int MiniAudioPlayer::getProgressPercent() const
{
  int total = getSongLengthSeconds();
  if (total <= 0)
  {
    return 0;
  }

  int pos = getCurrentPositionSeconds();
  return (pos * 100) / total;
}

bool MiniAudioPlayer::initVisualizerAudio()
{
  // grab node graph
  ma_node_graph *node_graph = nullptr;
  node_graph = ma_engine_get_node_graph(&audio_state.engine);
  ma_node *endpoint = ma_node_graph_get_endpoint(node_graph);

  // setup channels and config for splitter
  ma_uint32 channels = ma_engine_get_channels(&audio_state.engine);
  ma_splitter_node_config cfg = ma_splitter_node_config_init(channels);

  if (ma_splitter_node_init(node_graph, &cfg, NULL, &audio_state.splitter) != MA_SUCCESS)
  {
    Util::errorPrint("Splitter init failed");
    return false;
  }


  // splitter output 0 -> speakers
  if (ma_node_attach_output_bus(&audio_state.splitter, 0, endpoint, 0) != MA_SUCCESS)
  {
    Util::errorPrint("Failed to attach splitter output 0 to endpoint");
    return false;
  }
  // splitter output 1 -> sink
  if (ma_node_attach_output_bus(&audio_state.splitter, 1, endpoint, 0) != MA_SUCCESS)
  {
    Util::errorPrint("Failed to attach splitter output 1 to endpoint");
    return false;
  }

  Util::debugPrint("initVisualizerAudio completed successfully");
  return true;
}
