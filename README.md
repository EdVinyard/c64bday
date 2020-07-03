# Musical Birthday Marquee

Happy Birthday, John!

## Final Product

A finished Commodore 64 disk image is available in
[JohnBirthday2020.d64](JohnBirthday2020.d64), a scrolling birthday message
marquee that plays Happy Birthday. 

That disk image was built from the [Birthday](Birthday) directory.  The most 
useful Make targets are:

- `make test` - compile the code and start it immediately in VICE
- `make disk` - create a `.d64` 171 KB disk image

## Experimentation

Supporting and experimental features are in other subdirectories:

- [BigChar](BigChar) - scale C64 built-in font up to 8:1 (one char per pixel)

- [CustomCharSet](CustomCharSet) - replace the C64 build-in font bitmaps with
custom bitmaps

- [Marquee](Marquee) - giant, horizonally scrolling text

- [MusicExample](MusicExample) - play Happy Birthday two-voice music

- [SmoothScroll](SmoothScroll) - VIC-II hardware scrolling

- [SpriteExample](SpriteExampe) - VIC-II hardware sprites

- [tool](tool) - tools I didn't end up using much

## Development

These programs were written using [cc65](https://cc65.github.io/) and tested in
the [V.I.C.E.](https://vice-emu.sourceforge.io/) Commodore emulator on Ubuntu
18.04.
