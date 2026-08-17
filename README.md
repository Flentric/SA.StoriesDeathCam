# SA.StoriesDeathCam

A GTA: San Andreas ASI mod that recreates the LCS/VCS "Stories" **death camera**.
On WASTED the camera spins directly overhead and rises into the sky while the
screen washes to grey, replacing SA's default death cam. It also shortens the
death sequence to ~4 seconds and turns the hospital resurrection fade-in grey
instead of black.

![Stories death camera in action](assets/deathcam.gif)

Built with [plugin-sdk](https://github.com/DK22Pac/plugin-sdk). Targets
**GTA SA 1.0 US (HOODLUM)**.

## Install

Drop `SA.StoriesDeathCam.asi` and `SA.StoriesDeathCam.ini` into your game's
`scripts` folder (or any ASI-loader path — the INI must sit next to the ASI).
Requires an ASI loader (Silent's / Ultimate ASI Loader).

## Config

Everything is tunable live in `SA.StoriesDeathCam.ini` (it reloads while you
play): camera height / spin / rise, the grey wash timing and colour, and the
death sequence length (`WastedDelayMs`).

## Build

Needs the plugin-sdk with the `PLUGIN_SDK_DIR` environment variable set.
Build `SA.StoriesDeathCam.vcxproj` (configuration **Release GTA-SA**, platform
**Win32**, toolset **v145**).
