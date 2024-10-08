[English](docs/readme-en.md)

# obs-vlc-video-plugin

Модифицированный VLC-плагин с поддержкой [Streamlink](https://streamlink.github.io/) и аппаратного ускорения для OBS Studio.

<p align="center"><img width="80%" style="margin: 0" src="docs/assets/vlc-video-plugin-properties.png"></img></p>

## Установка

> Перед установкой сохраните резервную копию файлов `vlc-video.dll`, `vlc-video.pdb`. Путь по умолчанию `C:\Program Files\obs-studio\obs-plugins\64bit`.

1. [Установите VLC Player](https://www.videolan.org/vlc/)
2. [Установите Streamlink](https://streamlink.github.io/install.html).
3. Скачайте архив `vlc-plugin.zip` в разделе [релизов](https://github.com/Chimildic/obs-vlc-video-plugin/releases).
4. Распакуйте содержание архива в корень папки OBS Studio с заменой файлов. Путь по умолчанию `C:\Program Files\obs-studio`.

## Streamlink

Streamlink позволяет получить видеопоток через ссылку на канал или видео. Например, указав ссылку вида `https://www.twitch.tv/igorghk` плагин создаст отдельный процесс, в котором запустит локальный сервер. Также есть возможность использовать дополнительные параметры: низкая задержка, пропуск рекламы, личный токен и прочее. 

Используйте поле `Параметры Streamlink` для более тонкой настройки. Каждый параметр начинается с `--` и разделяется пробелом. К примеру `--hls-live-edge 1 --twitch-disable-ads`. Подробнее в [документации Streamlink](https://streamlink.github.io/cli.html).

## libVLC

libVLC - библиотека, которую использует плагин для связи с VLC плеером. Её также можно настравить через поле `Параметры VLC`. Их перечень в [документации](https://wiki.videolan.org/VLC_command-line_help). В частности [параметры для аппаратного ускорения](https://wiki.videolan.org/Documentation:Modules/avcodec/).

Каждый параметр имеет вид `:key=value` и разделяется пробелом. Например `:avcodec-skip-frame=1 :avcodec-hw=any`.

## Сборка

У плагина много внешних зависимостей, которых нет в этом репозитории. Для самостоятельной сборки `.dll` файла поместите модифицированный код в папку [vlc-video](https://github.com/obsproject/obs-studio/tree/master/plugins/vlc-video) из репозитория OBS Studio.

> Функция `get_free_port` требует добавления зависимости `Ws2_32.lib`. Путь для Visual Studio: `Свойства проекта > Компоновщик > Ввод > Дополнительные зависимости`.

## Обратная связь

- [Github Issues](https://github.com/Chimildic/obs-vlc-video-plugin/issues)
- [Телеграм](https://t.me/dev_chimildic)
- [Донат](https://yoomoney.ru/to/410014208620686)