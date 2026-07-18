# todo

- [X] different album ascii styled (current is half blocks)
 - uses traditional ascii now
- [X] equalizer that reacts to sound (underneath artist info)
  - FFT analyzer via custom miniaudio tap node + ring buffer
  - styles: block / ansi-art / braille / spectrogram (press `v` to cycle)
- [ ] m4a support via ffmpeg
- [X] directory sort alphabetically
- [ ] Faster directory traversal
 - [ ] ctrl-d to scroll half page down, ctrl-u scroll half page up (vim style)
 - [ ] / search for a song in subdir 
 - [ ] scrolling to the very top puts you straight to the bottom and vice versa
- [ ] remove unsupported files from directory listing
- [X] update help screen with new keybinds
  - [X] added '-' to go up in directory traversal (i like it from netrw)
- [ ] playlist support (create custom playlist or read those playlist files) 
- [ ] lua config
  - custom path to music default
  - prefered styles (detailed / block)
