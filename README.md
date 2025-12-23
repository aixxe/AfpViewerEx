# AfpViewerEx
[![LLVM MinGW (Linux)](https://github.com/aixxe/AfpViewerEx/actions/workflows/build-llvm.yml/badge.svg)](https://github.com/aixxe/AfpViewerEx/actions/workflows/build-llvm.yml)

A bunch of hooks thrown together to capture frames from `AfpViewerScene` as transparent PNG files

Needed this for exporting some customizations. I've only tested it with IIDX 32. Your mileage _will_ vary, etc.

## Usage

1. Enable the following patches before starting the game:
   - `Increase 'All Factory Settings' Buffer`
   - `Reroute 'All Factory Settings' Test Menu → AfpViewerScene`
1. Load `AfpViewerEx.dll` as a hook library in your game command-line arguments
1. Boot the game and select **ALL FACTORY SETTINGS** from the test menu to open the viewer
1. Select the animation you want to capture and press <kbd>Left</kbd> or <kbd>Right</kbd> to pause it
1. Press <kbd>F12</kbd> to start capturing, then press <kbd>Enter</kbd> to start playing from the beginning

Captured frames are stored in the `captures` directory next to the `AfpViewerEx.dll` library

You should be able to convert it to another format now, e.g. `.avif` which has fairly good [browser compatibility](https://caniuse.com/avif)

```
ffmpeg -r 60 -i frame_%05d.png -strict -1 -f yuv4mpegpipe -pix_fmt yuva444p - | avifenc --stdin --fps 60 output.avif
```

Framerate will vary between animations, so be careful to use the correct value when converting