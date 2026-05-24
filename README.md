# Orpheus: A C++ Music Player
A Terminal User Interface (TUI) music player that processes Music Files (MP3, M4A, FLAC and more) in a simplistic and stylish way.

## Build Dependencies

- ncurses
- C++ compiler (g++/clang)
- cmake
- miniaudio
- taglib
- lua
- libjpeg-turbo


## Installation

On Debian/Ubuntu:
```bash
sudo apt-get install taglib libncurses5-dev liblua5.3-dev libjpeg-turbo build-essential cmake pkg-config 

```
On Fedora:
```bash
sudo dnf install libmpdclient-devel ncurses-devel lua-devel libjpeg-turbo gcc cmake pkgconf
```

### Building
```
cd build
cmake ..
make
sudo make install
```

## Running
orpheus

### Configuration (TODO CHANGE)
Edit or add a orpheus.lua in ~/.config/orpheus/orpheus.lua
```bash
a real config would go here if there was one

```
