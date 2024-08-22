[English](docs/readme-en.md)

# obs-vlc-video-plugin

Модифицированный VLC-плагин с поддержкой [Streamlink](https://streamlink.github.io) и аппаратного ускорения для OBS Studio.

## Возможности

- Запуск видеопотока по ссылке на канал через локальный сервер от Streamlink с дополнительными параметрами. Не только Twitch.
- Поддержка аппаратного ускорения (перенос части нагрузки на видеокарту).
- Настройка выведена в интерфейс плагина.

<p align="center"><img width="85%" style="margin: 0" src="docs/assets/vlc-video-plugin-properties.png"></img></p>

## Установка

> Перед установкой сохраните резервную копию файлов `vlc-video.dll`, `vlc-video.pdb`. Путь по умолчанию `C:\Program Files\obs-studio\obs-plugins\64bit`.

1. [Установите Streamlink](https://streamlink.github.io/install.html).
2. Скачайте архив в разделе [релизов](https://github.com/Chimildic/obs-vlc-video-plugin/releases).
3. Распакуйте содержание архива в корень папки OBS Studio. Путь по умолчанию `C:\Program Files\obs-studio`.

## Streamlink

Streamlink позволяет получить видеопоток через ссылку на канал или видео. Например, указав ссылку вида `https://www.twitch.tv/igorghk` плагин создаст отдельный процесс, в котором запустит локальный сервер. Также есть возможность использовать дополнительные параметры: низкая задержка, пропуск рекламы, личный токен и прочее. 

Используйте поле `Параметры Streamlink` для более тонкой настройки. Каждый параметр начинается с `--` и разделяется пробелом. К примеру `--hls-live-edge 1 --twitch-disable-ads`. Подробнее в [документации Streamlink](https://streamlink.github.io/cli.html).

## libVLC

libVLC - библиотека, которую использует плагин для связи с VLC плеером. Её также можно настравить через поле `Параметры VLC`. Их перечень в [документации](https://wiki.videolan.org/VLC_command-line_help). В частности [параметры для аппаратного ускорения](https://wiki.videolan.org/Documentation:Modules/avcodec/).

Каждый параметр имеет вид `:key=value` и разделяется пробелом. Например `:avcodec-skip-frame=1 :avcodec-hw=any`.

## Сборка

У плагина много внешних зависимостей, которых нет в этом репозитории. Для самостоятельной сборки `.dll` файла поместите модифицированный код в папку [vlc-video](https://github.com/obsproject/obs-studio/tree/master/plugins/vlc-video) из репозитория OBS Studio.

> Функция `get_free_port` требует добавления зависимости `Ws2_32.lib`. Путь для Visual Studio: `Свойства проекта > Компоновщик > Ввод > Дополнительные зависимости`.