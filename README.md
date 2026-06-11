# Orpheus: A C++ Music Player
A Terminal User Interface (TUI) music player that processes Music Files (MP3, M4A, FLAC and more) in a simplistic and stylish way.

## Build Dependencies

- ncurses
- C++ compiler (g++/clang)
- cmake
- miniaudio
- taglib
- lua


## Installation

On Debian/Ubuntu:
```bash
sudo apt-get install taglib libncurses5-dev liblua5.3-dev build-essential cmake pkg-config
```

On Fedora:
```bash
sudo dnf install taglib-devel ncurses-devel lua-devel gcc cmake pkgconf
```

On Arch Linux:
```bash
sudo pacman -S taglib ncurses lua gcc cmake pkg-config
```

On macOS (Homebrew):
```bash
brew install taglib ncurses lua cmake pkg-config
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
