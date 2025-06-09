# C Music Player

A Terminal User Interface (TUI) music player that interfaces with MPD (Music Player Daemon).

## Features

- Connect to MPD server via Unix socket or TCP
- Create and manage playlists
- Add songs to the queue
- Basic playback controls (play, pause, stop, next, previous)
- Volume control
- Queue management
- Current song information display

## Dependencies

- libmpdclient
- ncurses
- C compiler (gcc/clang)
- make

## Building

```bash
make
```

## Running

```bash
./music_player
```

## Configuration

The player will attempt to connect to MPD using the default socket at `/run/user/1000/mpd/socket` or TCP connection at localhost:6600. You can modify these settings in the source code. 
