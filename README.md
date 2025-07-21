# Orpheus: A C Music Player
A Terminal User Interface (TUI) music player that interfaces with MPD (Music Player Daemon).
## Features

* Connect to MPD server via Unix socket or TCP
* Create and manage playlists
* Add songs to the queue
* Basic playback controls (play, pause, stop, next, previous)
* Volume control
* Queue management
* Current song information display

## Dependencies

* libmpdclient
* ncurses
* lua5.3
* C compiler (gcc/clang)
* cmake
* pkg-config

## Installation

On Debian/Ubuntu:
```bash
sudo apt-get install libmpdclient-dev libncurses5-dev liblua5.3-dev build-essential cmake pkg-config

```
On Fedora:
```bash
sudo dnf install libmpdclient-devel ncurses-devel lua-devel gcc cmake pkgconf
```
## Building
```
cd build
cmake ..
make
```
Alternatively, use the compile script:
`bash src/compile.sh`

## Running
./orpheus

### Configuration
Edit or add a config.lua
```bash 
───────┬────────────────────────────────────────────────────────────────────────────────────────────────
       │ File: src/config.lua
───────┼────────────────────────────────────────────────────────────────────────────────────────────────
   1   │ -- CBMP config
   2   │
   3   │ music_directory = "~/Music"
   4   │ connection_type = "socket" -- or "network"
   5   │ socket_path = "/home/bay/.config/mpd/socket"
   6   │ -- use these if you're on network
   7   │ -- host = "localhost"
   8   │ --port = 6600
───────┴──────────────────────────────────────────────────────────────────────────────────────────────
```

