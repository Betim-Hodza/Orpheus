// player.hpp
#pragma once
#include "art.hpp"
#include "miniaudio.h"
#include <atomic>
#include <string>
#include <taglib/fileref.h>
#include <vector>

struct SongMetadata
{
  std::string song_name;
  std::string artist_name;
  std::string song_path;
  std::string album_image_path;
  Art::ImageData cached_image;
};

struct PlayerData
{
  ma_engine engine;
  ma_sound sound;
  bool sound_is_initialized = false;
};

class MiniAudioPlayer
{
public:
  MiniAudioPlayer();
  ~MiniAudioPlayer();

  void init();
  void cleanup();

  // Legacy | play a single song by direct path (clears queue)
  bool startSong(const std::string& song_path);

  // Queue-based music playing
  void queueSong(const SongMetadata& song);           // add to end of queue
  void queueSongFirst(const SongMetadata& song);      // insert at beginning (play next)
  bool playCurrent();                                 // load & play current_index + 1; false if empty/overflow
  bool nextSong();                                    // stop current, advance index, play next
  bool prevSong();                                    // restart or go back to previous in queue
  void clearQueue();                                  // stop playing, reset everything
  int getCurrentIndex() const { return current_index; }
  size_t getQueueSize() const { return song_queue.size(); }
  bool isEmpty() const        { return song_queue.empty(); }

  bool pauseSong(); 
  bool rewind();   

  // Called from ui.cpp event loop: returns true when song has ended (for auto-advance)
  bool isCurrentEnded();
  void resetEndFlag();

  // Called from audio thread by miniaudio callback
  void onSongEnd();

  // Direct check using ma_sound_at_end (fallback if callback missed)
  bool isAtEnd() const;

  // Accessors for UI display
  const SongMetadata* getCurrentSong() const;
  std::string getCurrentSongPath() const;
  const SongMetadata* getQueueSong(unsigned int index) const;

  // Progress / timing (seconds)
  int getCurrentPositionSeconds() const;
  int getSongLengthSeconds() const;
  int getProgressPercent() const;

private:
  PlayerData audio_state;
  int current_index = -1;                // index into song_queue of what's playing
  int song_length_pcm = 0;               // cached length in pcm frames for comparison
  std::atomic<bool> just_ended{false};   // true for one frame after song ends
  std::vector<SongMetadata> song_queue;  // playlist

  void stopCurrent();
  bool loadSong(const std::string& path);
};


