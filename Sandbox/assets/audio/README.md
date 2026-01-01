# Audio Assets Directory

This directory contains audio files used by the Pillar Engine audio demos.

## Required Audio Files

To use the Audio Demo scene, place the following WAV files in this directory:

### Background Music
- **background_music.wav** - Looping background music (recommended: 30-120 seconds)

### Sound Effects
- **sfx_1.wav** - Sound effect 1 (used for moving sound demo)
- **sfx_2.wav** - Sound effect 2 (left position)
- **sfx_3.wav** - Sound effect 3 (right position)
- **sfx_4.wav** - Sound effect 4 (front position)

## Audio Format Requirements

- **Format**: WAV (Waveform Audio File Format)
- **Encoding**: PCM (Pulse Code Modulation)
- **Bit Depth**: 8-bit or 16-bit
- **Channels**: Mono or Stereo
- **Sample Rate**: Any (44100 Hz recommended)

## Where to Find Audio Files

You can create or find WAV audio files from various sources:

1. **Free Sound Libraries**:
   - [Freesound.org](https://freesound.org/) - CC-licensed sounds
   - [Zapsplat.com](https://www.zapsplat.com/) - Free SFX
   - [OpenGameArt.org](https://opengameart.org/) - Game audio

2. **Create Your Own**:
   - Use Audacity (free audio editor) to create/convert audio
   - Record your own sounds
   - Generate tones/effects

3. **Convert Other Formats**:
   - Use Audacity to convert MP3/OGG to WAV
   - Use FFmpeg: `ffmpeg -i input.mp3 output.wav`

## Subdirectories

You can organize audio files into subdirectories:

- `audio/sfx/` - Sound effects
- `audio/music/` - Background music
- `audio/voice/` - Voice clips

The AssetManager will automatically search these locations.

## Tips for Audio Demo

- **Background Music**: Use a calm, looping track (5-30 seconds is fine)
- **Sound Effects**: Short sounds work best (0.1-2 seconds)
- **3D Audio**: Clear, distinct sounds help demonstrate spatial audio
- **File Size**: Keep files small (<1MB each) for quick loading

## Example Using Audacity

1. Generate a tone: **Generate ? Tone**
2. Set duration: 1 second
3. Export: **File ? Export ? Export as WAV**
4. Settings: PCM 16-bit, 44100 Hz

## Troubleshooting

If audio doesn't play:

1. Check file names match exactly (case-sensitive)
2. Ensure files are in correct format (WAV, PCM)
3. Check console logs for loading errors
4. Verify audio engine initialized successfully

---

**Note**: This directory is created automatically during build.
Place your audio files here before running the audio demos.
