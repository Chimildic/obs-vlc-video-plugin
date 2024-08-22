# obs-vlc-video-plugin

Modified VLC-plugin with [Streamlink](https://streamlink.github.io) and hardware acceleration support for OBS Studio.

## Features

- Launch video stream via link to channel by local server from Streamline with additional parameters. Twitch and more.
- Hardware acceleration support (move part of the load to the graphics card).
- Custom UI

<p align="center"><img width="85%" style="margin: 0" src="assets/vlc-video-plugin-properties-en.png"></img></p>

## Install

> Save backup of the files before install: `vlc-video.dll`, `vlc-video.pdb`. Default path `C:\Program Files\obs-studio\obs-plugins\64bit`.

1. [Install Streamlink](https://streamlink.github.io/install.html).
2. Download archive in [releases](https://github.com/Chimildic/obs-vlc-video-plugin/releases).
3. Unpack archive to root folder of OBS Studio. Default path `C:\Program Files\obs-studio`.

## Streamlink

Streamlink allows to receive a video stream via link to channel. For example, by link like `https://www.twitch.tv/igorghk` the plugin will create a new process in which to run the local server. It is also possible to use additional parameters: low latency, skip twitch ads, personal token and more.

Use the field `Streamlink options` to fine-tuning. Each parameter starts with `--` and is separated by space. For example `--hls-live-edge 1 --twitch-disable-ads`. See [Streamlink documentation](https://streamlink.github.io/cli.html).

## libVLC

libVLC is library that the plugin uses to communicate with the VLC player. It can also be configured via the `VLC options` field. Their list is in the [documentation](https://wiki.videolan.org/VLC_command-line_help). In particular [parameters for hardware acceleration](https://wiki.videolan.org/Documentation:Modules/avcodec/).

The each option looks like `:key=value` and is separated by space `:avcodec-skip-frame=1 :avcodec-hw=any`.

## Build

The plugin has many dependencies that not provided here. In order to build `.dll` youself, place modified files to [vlc-video](https://github.com/obsproject/obs-studio/tree/master/plugins/vlc-video) folder from OBS Studio repository.

> The `get_free_port` function requires `Ws2_32.lib` dependency. The path for Visual Studio: `Project Properties > Linker > Input > Additional Dependencies`.