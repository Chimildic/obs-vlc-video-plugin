#include "plugin.h"
#include <media-io/video-frame.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <Windows.h>

#define do_log(level, format, ...)            \
	blog(level, "[vlc_source: '%s'] " format, \
		 obs_source_get_name(ss->source), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)

/* clang-format off */

#define S_VLC_GROUP                    "vlc_group"
#define S_PLAYLIST                     "playlist"
#define S_LOOP                         "loop"
#define S_SHUFFLE                      "shuffle"
#define S_BEHAVIOR                     "playback_behavior_int"
#define S_NETWORK_CACHING              "network_caching"
#define S_TRACK                        "track"
#define S_SUBTITLE_ENABLE              "subtitle_enable"
#define S_SUBTITLE_TRACK               "subtitle"
#define S_VLC_HW                       "hardware_acceleration"
#define S_VLC_HW_ANY                   "any"
#define S_VLC_HW_DXVA2                 "dxva2"
#define S_VLC_HW_D3D11                 "d3d11va"
#define S_VLC_HW_NONE                  "none"
#define S_VLC_SKIP_B_FRAMES            "skip_b_frames"
#define S_VLC_REST_OPTIONS             "vlc_rest_options"
#define S_SL_ENABLE                    "streamlink_enable"
#define S_SL_URL                       "streamlink_url"
#define S_SL_TWITCH_LOW_LATENCY        "streamlink_twitch_low_latency"
#define S_SL_TWITCH_DISABLE_ADS        "streamlink_twitch_disable_ads"
#define S_SL_SHOW_CMD                  "streamlink_show_cmd"
#define S_SLQ                          "streamlink_quality"
#define S_SLQ_BEST                     "best"
#define S_SLQ_WORST                    "worst"
#define S_SLQ_AUDIO                    "audio_only"
#define S_SLQ_1080P                    "1080p"
#define S_SLQ_720P                     "720p"
#define S_SLQ_480P                     "480p"
#define S_SLQ_360P                     "360p"
#define S_SLQ_160P                     "160p"
#define S_SL_REST_OPTIONS              "streamlink_rest_options"
#define S_GITHUB_BUTTON                "github_button"
#define S_AM                           "audio_monitoring"

#define T_(text) obs_module_text(text)
#define T_VLC_GROUP                    T_("VLCGroup")
#define T_PLAYLIST                     T_("Playlist")
#define T_LOOP                         T_("LoopPlaylist")
#define T_SHUFFLE                      T_("Shuffle")
#define T_BEHAVIOR                     T_("PlaybackBehavior")
#define T_BEHAVIOR_STOP_RESTART        T_("PlaybackBehavior.StopRestart")
#define T_BEHAVIOR_PAUSE_UNPAUSE       T_("PlaybackBehavior.PauseUnpause")
#define T_BEHAVIOR_ALWAYS_PLAY         T_("PlaybackBehavior.AlwaysPlay")
#define T_NETWORK_CACHING              T_("NetworkCaching")
#define T_TRACK                        T_("AudioTrack")
#define T_SUBTITLE_ENABLE              T_("SubtitleEnable")
#define T_SUBTITLE_TRACK               T_("SubtitleTrack")
#define T_VLC_HW                       T_("VLC.HW")
#define T_VLC_HW_ANY                   T_("VLC.HW.ANY")
#define T_VLC_HW_DXVA2                 T_("VLC.HW.DXVA2")
#define T_VLC_HW_D3D11                 T_("VLC.HW.D3D11")
#define T_VLC_HW_NONE                  T_("VLC.HW.NONE")
#define T_VLC_SKIP_B_FRAMES            T_("VLC.SkipBFrames")
#define T_VLC_REST_OPTIONS             T_("VLC.RestOptions")
#define T_VLC_REST_OPTIONS_DESCRIPTION T_("VLC.RestOptions.Description")
#define T_SL_ENABLE                    T_("Streamlink.Enable")
#define T_SL_URL                       T_("Streamlink.URL")
#define T_SL_TWITCH_LOW_LATENCY        T_("Streamlink.TwitchLowLatency")
#define T_SL_TWITCH_LOW_LATENCY_DESCRIPTION T_("Streamlink.TwitchLowLatency.Description")
#define T_SL_TWITCH_DISABLE_ADS        T_("Streamlink.TwitchDisableAds")
#define T_SL_TWITCH_DISABLE_ADS_DESCRIPTION T_("Streamlink.TwitchDisableAds.Description")
#define T_SL_SHOW_CMD                  T_("Streamlink.ShowCMD")
#define T_SLQ                          T_("Streamlink.Quality")
#define T_SLQ_BEST                     T_("Streamlink.Quality.Best")
#define T_SLQ_WORST                    T_("Streamlink.Quality.Worst")
#define T_SLQ_AUDIO                    T_("Streamlink.Quality.AudioOnly")
#define T_SLQ_1080P                    T_("1080p")
#define T_SLQ_720P                     T_("720p")
#define T_SLQ_480P                     T_("480p")
#define T_SLQ_360P                     T_("360p")
#define T_SLQ_160P                     T_("160p")
#define T_SL_REST_OPTIONS              T_("Streamlink.RestOptions")
#define T_SL_REST_OPTIONS_DESCRIPTION  T_("Streamlink.RestOptions.Description")
#define T_GITHUB_BUTTON                T_("Открыть GitHub")
#define T_AM                           T_("AudioMonitoring")
#define T_AM_NONE                      T_("AudioMonitoring.None")
#define T_AM_MONITOR_ONLY              T_("AudioMonitoring.MonitorOnly")
#define T_AM_BOTH                      T_("AudioMonitoring.Both")

/* clang-format on */

/* ------------------------------------------------------------------------- */

struct media_file_data
{
	char *path;
	libvlc_media_t *media;
	PROCESS_INFORMATION process_information;
};

typedef DARRAY(struct media_file_data) media_file_array_t;

enum behavior
{
	BEHAVIOR_STOP_RESTART,
	BEHAVIOR_PAUSE_UNPAUSE,
	BEHAVIOR_ALWAYS_PLAY,
};

struct vlc_config
{
	int network_caching;
	int track_index;
	int subtitle_index;
	bool subtitle_enable;
	char *hw_value;
	bool skip_b_frames;
	char *rest_options;
};

struct streamlink_config
{
	bool enable;
	char *url;
	char *quality;
	bool twitch_low_latency_enable;
	bool twitch_disable_ads;
	bool show_cmd;
	char *rest_options;
};

typedef struct vlc_config vlc_config_t;
typedef struct streamlink_config streamlink_config_t;

struct vlc_source
{
	obs_source_t *source;
	vlc_config_t vlc_config;
	streamlink_config_t streamlink_config;

	libvlc_media_player_t *media_player;
	libvlc_media_list_player_t *media_list_player;

	struct obs_source_frame frame;
	struct obs_source_audio audio;
	size_t audio_capacity;

	pthread_mutex_t mutex;
	media_file_array_t files;
	enum behavior behavior;
	bool loop;
	bool shuffle;

	obs_hotkey_id play_pause_hotkey;
	obs_hotkey_id restart_hotkey;
	obs_hotkey_id stop_hotkey;
	obs_hotkey_id playlist_next_hotkey;
	obs_hotkey_id playlist_prev_hotkey;
};

static wchar_t *convertCharArrayToLPCWSTR(const char *charArray)
{
	int size = MultiByteToWideChar(CP_UTF8, 0, charArray, -1, NULL, 0);
	wchar_t *lpwstr = (wchar_t *)malloc(size * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, charArray, -1, lpwstr, size);
	return lpwstr;
}

static PROCESS_INFORMATION create_process(const char *command, bool show_cmd)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	wchar_t *lpcwstr_command = convertCharArrayToLPCWSTR(command);
	if (!CreateProcess(NULL, lpcwstr_command, NULL, NULL, TRUE, show_cmd ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	return pi;
}

static void terminate_process(PROCESS_INFORMATION pi)
{
	if (pi.hProcess != NULL)
	{
		TerminateProcess(pi.hProcess, 0);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

static void terminate_process_array(media_file_array_t *files)
{
	for (size_t i = 0; i < files->num; i++)
	{
		terminate_process(files->array[i].process_information);
	}
}

static int get_free_port()
{
	SOCKET sockfd;
	struct sockaddr_in addr;
	int addr_len = sizeof(addr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(0);

	if (bind(sockfd, (SOCKADDR *)&addr, sizeof(addr)) == -1)
	{
		closesocket(sockfd);
		return -1;
	}

	if (getsockname(sockfd, (SOCKADDR *)&addr, &addr_len) == -1)
	{
		closesocket(sockfd);
		return -1;
	}

	int free_port = ntohs(addr.sin_port);
	closesocket(sockfd);
	return free_port;
}

static bool github_button_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	ShellExecute(0, 0, L"https://github.com/Chimildic/obs-vlc-video-plugin", 0, 0, SW_SHOW);
	return true;
}

static bool audio_monitoring_changed(void *data, obs_properties_t *props, obs_property_t *list, obs_data_t *settings)
{
	struct vlc_source *vlc_source = data;
	int monitoring_type = (int)obs_data_get_int(settings, S_AM);
	obs_source_set_monitoring_type(vlc_source->source, monitoring_type);
	return true;
}

static libvlc_media_t *get_media(media_file_array_t *files, const char *path)
{
	libvlc_media_t *media = NULL;

	for (size_t i = 0; i < files->num; i++)
	{
		const char *cur_path = files->array[i].path;

		if (strcmp(path, cur_path) == 0)
		{
			media = files->array[i].media;
			libvlc_media_retain_(media);
			break;
		}
	}

	return media;
}

static inline libvlc_media_t *create_media_from_file(const char *file)
{
	return (file && strstr(file, "://") != NULL)
			   ? libvlc_media_new_location_(libvlc, file)
			   : libvlc_media_new_path_(libvlc, file);
}

static void replace_vlc_config(struct vlc_config *current_config, struct vlc_config *new_config)
{
	free(current_config->hw_value);
	free(current_config->rest_options);
	*current_config = *new_config;
}

static void replace_streamling_config(struct streamlink_config *current_config, struct streamlink_config *new_config)
{
	free(current_config->url);
	free(current_config->quality);
	free(current_config->rest_options);
	*current_config = *new_config;
}

static void free_files(media_file_array_t *files)
{
	for (size_t i = 0; i < files->num; i++)
	{
		bfree(files->array[i].path);
		libvlc_media_release_(files->array[i].media);
	}
	da_free(*files);
}

#define MAKEFORMAT(ch0, ch1, ch2, ch3)                            \
	((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
	 ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))

static inline bool chroma_is(const char *chroma, const char *val)
{
	return *(uint32_t *)chroma == *(uint32_t *)val;
}

static enum video_format convert_vlc_video_format(char *chroma, bool *full)
{
	*full = false;

#define CHROMA_TEST(val, ret)   \
	if (chroma_is(chroma, val)) \
	return ret
#define CHROMA_CONV(val, new_val, ret)               \
	do                                               \
	{                                                \
		if (chroma_is(chroma, val))                  \
		{                                            \
			*(uint32_t *)chroma = (uint32_t)new_val; \
			return ret;                              \
		}                                            \
	} while (false)
#define CHROMA_CONV_FULL(val, new_val, ret) \
	do                                      \
	{                                       \
		*full = true;                       \
		CHROMA_CONV(val, new_val, ret);     \
	} while (false)

	CHROMA_TEST("RGBA", VIDEO_FORMAT_RGBA);
	CHROMA_TEST("BGRA", VIDEO_FORMAT_BGRA);

	/* 4:2:0 formats */
	CHROMA_TEST("NV12", VIDEO_FORMAT_NV12);
	CHROMA_TEST("I420", VIDEO_FORMAT_I420);
	CHROMA_TEST("IYUV", VIDEO_FORMAT_I420);
	CHROMA_CONV("NV21", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_NV12);
	CHROMA_CONV("I422", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_NV12);
	CHROMA_CONV("Y42B", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_NV12);
	CHROMA_CONV("YV12", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_NV12);
	CHROMA_CONV("yv12", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_NV12);

	CHROMA_CONV_FULL("J420", MAKEFORMAT('J', '4', '2', '0'), VIDEO_FORMAT_I420);

	/* 4:2:2 formats */
	CHROMA_TEST("UYVY", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("UYNV", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("UYNY", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("Y422", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("HDYC", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("AVUI", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("uyv1", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("2vuy", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("2Vuy", VIDEO_FORMAT_UYVY);
	CHROMA_TEST("2Vu1", VIDEO_FORMAT_UYVY);

	CHROMA_TEST("YUY2", VIDEO_FORMAT_YUY2);
	CHROMA_TEST("YUYV", VIDEO_FORMAT_YUY2);
	CHROMA_TEST("YUNV", VIDEO_FORMAT_YUY2);
	CHROMA_TEST("V422", VIDEO_FORMAT_YUY2);

	CHROMA_TEST("YVYU", VIDEO_FORMAT_YVYU);

	CHROMA_CONV("v210", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("cyuv", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("CYUV", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("VYUY", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("NV16", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("NV61", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("I410", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("I422", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("Y42B", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("J422", MAKEFORMAT('U', 'Y', 'V', 'Y'), VIDEO_FORMAT_UYVY);

	/* 4:4:4 formats */
	CHROMA_TEST("I444", VIDEO_FORMAT_I444);
	CHROMA_CONV_FULL("J444", MAKEFORMAT('R', 'G', 'B', 'A'), VIDEO_FORMAT_RGBA);
	CHROMA_CONV("YUVA", MAKEFORMAT('R', 'G', 'B', 'A'), VIDEO_FORMAT_RGBA);

	/* 4:4:0 formats */
	CHROMA_CONV("I440", MAKEFORMAT('I', '4', '4', '4'), VIDEO_FORMAT_I444);
	CHROMA_CONV("J440", MAKEFORMAT('I', '4', '4', '4'), VIDEO_FORMAT_I444);

	/* 4:1:0 formats */
	CHROMA_CONV("YVU9", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("I410", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_UYVY);

	/* 4:1:1 formats */
	CHROMA_CONV("I411", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_UYVY);
	CHROMA_CONV("Y41B", MAKEFORMAT('N', 'V', '1', '2'), VIDEO_FORMAT_UYVY);

	/* greyscale formats */
	CHROMA_TEST("GREY", VIDEO_FORMAT_Y800);
	CHROMA_TEST("Y800", VIDEO_FORMAT_Y800);
	CHROMA_TEST("Y8  ", VIDEO_FORMAT_Y800);
#undef CHROMA_CONV_FULL
#undef CHROMA_CONV
#undef CHROMA_TEST

	*(uint32_t *)chroma = (uint32_t)MAKEFORMAT('B', 'G', 'R', 'A');
	return VIDEO_FORMAT_BGRA;
}

static inline unsigned get_format_lines(enum video_format format, unsigned height, size_t plane)
{
	switch (format)
	{
	case VIDEO_FORMAT_I420:
	case VIDEO_FORMAT_NV12:
		return (plane == 0) ? height : height / 2;
	case VIDEO_FORMAT_YVYU:
	case VIDEO_FORMAT_YUY2:
	case VIDEO_FORMAT_UYVY:
	case VIDEO_FORMAT_I444:
	case VIDEO_FORMAT_RGBA:
	case VIDEO_FORMAT_BGRA:
	case VIDEO_FORMAT_BGRX:
	case VIDEO_FORMAT_Y800:
		return height;
	case VIDEO_FORMAT_NONE:
	default:
		break;
	}
	return 0;
}

static enum audio_format convert_vlc_audio_format(char *format)
{
#define AUDIO_TEST(val, ret)    \
	if (chroma_is(format, val)) \
	return ret
#define AUDIO_CONV(val, new_val, ret)                \
	do                                               \
	{                                                \
		if (chroma_is(format, val))                  \
		{                                            \
			*(uint32_t *)format = (uint32_t)new_val; \
			return ret;                              \
		}                                            \
	} while (false)

	AUDIO_TEST("S16N", AUDIO_FORMAT_16BIT);
	AUDIO_TEST("S32N", AUDIO_FORMAT_32BIT);
	AUDIO_TEST("FL32", AUDIO_FORMAT_FLOAT);

	AUDIO_CONV("U16N", MAKEFORMAT('S', '1', '6', 'N'), AUDIO_FORMAT_16BIT);
	AUDIO_CONV("U32N", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
	AUDIO_CONV("S24N", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
	AUDIO_CONV("U24N", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
	AUDIO_CONV("FL64", MAKEFORMAT('F', 'L', '3', '2'), AUDIO_FORMAT_FLOAT);

	AUDIO_CONV("S16I", MAKEFORMAT('S', '1', '6', 'N'), AUDIO_FORMAT_16BIT);
	AUDIO_CONV("U16I", MAKEFORMAT('S', '1', '6', 'N'), AUDIO_FORMAT_16BIT);
	AUDIO_CONV("S24I", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
	AUDIO_CONV("U24I", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
	AUDIO_CONV("S32I", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
	AUDIO_CONV("U32I", MAKEFORMAT('S', '3', '2', 'N'), AUDIO_FORMAT_32BIT);
#undef AUDIO_CONV
#undef AUDIO_TEST

	*(uint32_t *)format = (uint32_t)MAKEFORMAT('F', 'L', '3', '2');
	return AUDIO_FORMAT_FLOAT;
}

/* ------------------------------------------------------------------------- */

static void vlcs_get_metadata(void *data, calldata_t *cd)
{
	struct vlc_source *vlcs = data;
	const char *data_id = calldata_string(cd, "tag_id");

	if (!vlcs || !data_id)
	{
		return;
	}

	libvlc_media_t *media = libvlc_media_player_get_media_(vlcs->media_player);
	if (!media)
	{
		return;
	}

#define VLC_META(media, cd, did, tid, tag)                                       \
	else if (strcmp(did, tid) == 0)                                              \
	{                                                                            \
		calldata_set_string(cd, "tag_data", libvlc_media_get_meta_(media, tag)); \
	}

	if (strcmp(data_id, "title") == 0)
	{
		calldata_set_string(cd, "tag_data", libvlc_media_get_meta_(media, libvlc_meta_Title));
	}

	VLC_META(media, cd, data_id, "artist", libvlc_meta_Artist)
	VLC_META(media, cd, data_id, "genre", libvlc_meta_Genre)
	VLC_META(media, cd, data_id, "copyright", libvlc_meta_Copyright)
	VLC_META(media, cd, data_id, "album", libvlc_meta_Album)
	VLC_META(media, cd, data_id, "track_number", libvlc_meta_TrackNumber)
	VLC_META(media, cd, data_id, "description", libvlc_meta_Description)
	VLC_META(media, cd, data_id, "rating", libvlc_meta_Rating)
	VLC_META(media, cd, data_id, "date", libvlc_meta_Date)
	VLC_META(media, cd, data_id, "setting", libvlc_meta_Setting)
	VLC_META(media, cd, data_id, "url", libvlc_meta_URL)
	VLC_META(media, cd, data_id, "language", libvlc_meta_Language)
	VLC_META(media, cd, data_id, "now_playing", libvlc_meta_NowPlaying)
	VLC_META(media, cd, data_id, "publisher", libvlc_meta_Publisher)
	VLC_META(media, cd, data_id, "encoded_by", libvlc_meta_EncodedBy)
	VLC_META(media, cd, data_id, "artwork_url", libvlc_meta_ArtworkURL)
	VLC_META(media, cd, data_id, "track_id", libvlc_meta_TrackID)
	VLC_META(media, cd, data_id, "track_total", libvlc_meta_TrackTotal)
	VLC_META(media, cd, data_id, "director", libvlc_meta_Director)
	VLC_META(media, cd, data_id, "season", libvlc_meta_Season)
	VLC_META(media, cd, data_id, "episode", libvlc_meta_Episode)
	VLC_META(media, cd, data_id, "show_name", libvlc_meta_ShowName)
	VLC_META(media, cd, data_id, "actors", libvlc_meta_Actors)
	VLC_META(media, cd, data_id, "album_artist", libvlc_meta_AlbumArtist)
	VLC_META(media, cd, data_id, "disc_number", libvlc_meta_DiscNumber)
	VLC_META(media, cd, data_id, "disc_total", libvlc_meta_DiscTotal)

	libvlc_media_release_(media);
#undef VLC_META
}

/* ------------------------------------------------------------------------- */

static void vlcs_destroy(void *data)
{
	struct vlc_source *c = data;
	terminate_process_array(&(c->files));

	if (c->media_list_player)
	{
		libvlc_media_list_player_stop_(c->media_list_player);
		libvlc_media_list_player_release_(c->media_list_player);
	}
	if (c->media_player)
	{
		libvlc_media_player_release_(c->media_player);
	}

	bfree((void *)c->audio.data[0]);
	obs_source_frame_free(&c->frame);

	free_files(&c->files);
	pthread_mutex_destroy(&c->mutex);
	bfree(c);
}

static void *vlcs_video_lock(void *data, void **planes)
{
	struct vlc_source *c = data;
	for (size_t i = 0; i < MAX_AV_PLANES && c->frame.data[i] != NULL; i++)
	{
		planes[i] = c->frame.data[i];
	}
	return NULL;
}

static void vlcs_video_display(void *data, void *picture)
{
	struct vlc_source *c = data;
	c->frame.timestamp = (uint64_t)libvlc_clock_() * 1000ULL - time_start;
	obs_source_output_video(c->source, &c->frame);

	UNUSED_PARAMETER(picture);
}

static void calculate_display_size(struct vlc_source *c, unsigned *width, unsigned *height)
{
	libvlc_media_t *media = libvlc_media_player_get_media_(c->media_player);
	if (!media)
	{
		return;
	}

	libvlc_media_track_t **tracks;

	unsigned count = libvlc_media_tracks_get_(media, &tracks);

	if (count > 0)
	{
		for (unsigned i = 0; i < count; i++)
		{
			libvlc_media_track_t *track = tracks[i];

			if (track->i_type != libvlc_track_video)
			{
				continue;
			}

			unsigned display_width = track->video->i_width;
			unsigned display_height = track->video->i_height;

			if (display_width == 0 || display_height == 0)
			{
				continue;
			}

			/* Adjust for Sample Aspect Ratio (SAR) */
			if (track->video->i_sar_num > 0 && track->video->i_sar_den > 0)
			{
				display_width = (unsigned)util_mul_div64(display_width, track->video->i_sar_num, track->video->i_sar_den);
			}

			switch (track->video->i_orientation)
			{
			case libvlc_video_orient_left_top:
			case libvlc_video_orient_left_bottom:
			case libvlc_video_orient_right_top:
			case libvlc_video_orient_right_bottom:
				/* orientation swaps height and width */
				*width = display_height;
				*height = display_width;
				break;
			default:
				/* height and width not swapped */
				*width = display_width;
				*height = display_height;
				break;
			}
		}
		libvlc_media_tracks_release_(tracks, count);
	}
	libvlc_media_release_(media);
}

static unsigned vlcs_video_format(void **p_data, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
	struct vlc_source *c = *p_data;
	enum video_format new_format;
	enum video_range_type range;
	bool new_range;
	size_t i = 0;

	new_format = convert_vlc_video_format(chroma, &new_range);

	/* The width and height passed from VLC are the buffer size rather than
	 * the correct video display size, and may be the next multiple of 32
	 * up from the original dimension, e.g. 1080 would become 1088. VLC 4.0
	 * will pass the correct display size in *(width+1) and *(height+1) but
	 * for now we need to calculate it ourselves. */
	calculate_display_size(c, width, height);

	/* don't allocate a new frame if format/width/height hasn't changed */
	if (c->frame.format != new_format || c->frame.width != *width || c->frame.height != *height)
	{
		obs_source_frame_free(&c->frame);
		obs_source_frame_init(&c->frame, new_format, *width, *height);

		c->frame.format = new_format;
		c->frame.full_range = new_range;
		range = c->frame.full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
		video_format_get_parameters_for_format(
			VIDEO_CS_DEFAULT, range, new_format,
			c->frame.color_matrix, c->frame.color_range_min,
			c->frame.color_range_max);
	}

	while (c->frame.data[i])
	{
		pitches[i] = (unsigned)c->frame.linesize[i];
		lines[i] = get_format_lines(c->frame.format, *height, i);
		i++;
	}

	return 1;
}

static void vlcs_audio_play(void *data, const void *samples, unsigned count, int64_t pts)
{
	struct vlc_source *c = data;
	size_t size = get_audio_size(c->audio.format, c->audio.speakers, count);

	if (c->audio_capacity < count)
	{
		c->audio.data[0] = brealloc((void *)c->audio.data[0], size);
		c->audio_capacity = count;
	}

	memcpy((void *)c->audio.data[0], samples, size);
	c->audio.timestamp = (uint64_t)pts * 1000ULL - time_start;
	c->audio.frames = count;

	obs_source_output_audio(c->source, &c->audio);
}

static int vlcs_audio_setup(void **p_data, char *format, unsigned *rate, unsigned *channels)
{
	struct vlc_source *c = *p_data;
	enum audio_format new_audio_format;
	struct obs_audio_info aoi;
	obs_get_audio_info(&aoi);
	uint32_t out_channels = get_audio_channels(aoi.speakers);

	new_audio_format = convert_vlc_audio_format(format);
	if (*channels > out_channels)
	{
		*channels = out_channels;
	}

	/* don't free audio data if the data is the same format */
	if (c->audio.format == new_audio_format &&
		c->audio.samples_per_sec == *rate &&
		c->audio.speakers == (enum speaker_layout) * channels)
	{
		return 0;
	}

	c->audio_capacity = 0;
	bfree((void *)c->audio.data[0]);

	memset(&c->audio, 0, sizeof(c->audio));
	c->audio.speakers = (enum speaker_layout) * channels;
	c->audio.samples_per_sec = *rate;
	c->audio.format = new_audio_format;
	return 0;
}

static void run_streamlink_server(struct media_file_data *data, const char *url, struct streamlink_config *config)
{
	char command[4096] = "streamlink --player-external-http --player-external-http-interface 127.0.0.1";
	if (strstr(config->rest_options, "--url") == NULL)
	{
		strcat(command, " --url ");
		strcat(command, url);
	}
	if (strstr(config->rest_options, "--default-stream") == NULL)
	{
		char *fallback_quality = strstr("best,1080p60,1080p,936p60,936p,720p60,720p,480p,360p,160p,worst", config->quality);
		strcat(command, " --default-stream ");
		strcat(command, fallback_quality);
	}

	int port;
	const char *port_option = strstr(config->rest_options, "--player-external-http-port");
	if (port_option == NULL)
	{
		port = get_free_port();
		snprintf(command, sizeof(command), "%s --player-external-http-port %d", command, port);
	}
	else
	{
		port = atoi(&port_option[sizeof("--player-external-http-port")]);
	}
	config->twitch_low_latency_enable &&strcat(command, " --twitch-low-latency");
	config->twitch_disable_ads &&strcat(command, " --twitch-disable-ads");
	snprintf(command, sizeof(command), "%s %s", command, config->rest_options);

	terminate_process(data->process_information);
	data->process_information = create_process(command, config->show_cmd);

	char server_url[25];
	snprintf(server_url, sizeof(server_url), "http://127.0.0.1:%d/", port);
	data->path = server_url;
}

static bool valid_extension(const char *ext)
{
	struct dstr test = {0};
	bool valid = false;
	const char *b;
	const char *e;

	if (!ext || !*ext)
		return false;

	b = &EXTENSIONS_MEDIA[1];
	e = strchr(b, ';');

	for (;;)
	{
		if (e)
			dstr_ncopy(&test, b, e - b);
		else
			dstr_copy(&test, b);

		if (dstr_cmpi(&test, ext) == 0)
		{
			valid = true;
			break;
		}

		if (!e)
			break;

		b = e + 2;
		e = strchr(b, ';');
	}

	dstr_free(&test);
	return valid;
}

static void add_file(struct vlc_source *c, media_file_array_t *new_files, const char *path)
{
	struct vlc_config *vlc_config = &c->vlc_config;
	struct streamlink_config *streamlink_config = &c->streamlink_config;

	struct dstr new_path = {0};
	dstr_copy(&new_path, path);
	bool is_url = path && strstr(path, "://") != NULL;
#ifdef _WIN32
	if (!is_url)
	{
		dstr_replace(&new_path, "/", "\\");
	}
#endif
	struct media_file_data data = {0};
	if (is_url && streamlink_config->enable)
	{
		run_streamlink_server(&data, new_path.array, streamlink_config);
		dstr_copy(&new_path, data.path);
	}

	libvlc_media_t *new_media = get_media(&c->files, new_path.array);
	if (!new_media)
	{
		new_media = get_media(new_files, new_path.array);
	}
	if (!new_media)
	{
		new_media = create_media_from_file(new_path.array);
	}

	if (new_media)
	{
		char buffer[512];
		snprintf(buffer, sizeof(buffer), ":audio-track=%d", vlc_config->track_index - 1);
		libvlc_media_add_option_(new_media, buffer);

		snprintf(buffer, sizeof(buffer), ":avcodec-hw=%s", vlc_config->hw_value);
		libvlc_media_add_option_(new_media, buffer);

		if (is_url)
		{
			snprintf(buffer, sizeof(buffer), ":network-caching=%d", vlc_config->network_caching);
			libvlc_media_add_option_(new_media, buffer);
		}
		if (vlc_config->subtitle_enable)
		{
			snprintf(buffer, sizeof(buffer), ":sub-track=%d", vlc_config->subtitle_index - 1);
			libvlc_media_add_option_(new_media, buffer);
		}
		if (vlc_config->skip_b_frames)
		{
			libvlc_media_add_option_(new_media, ":avcodec-skip-frame=1");
		}

		strcpy(buffer, vlc_config->rest_options);
		char *next_token = NULL;
		char *option = strtok_s(buffer, " ", &next_token);
		while (option != NULL)
		{
			libvlc_media_add_option_(new_media, option);
			option = strtok_s(NULL, " ", &next_token);
		}

		data.path = new_path.array;
		data.media = new_media;
		da_push_back(*new_files, &data);
	}
	else
	{
		dstr_free(&new_path);
	}
}

static void add_file_array(struct vlc_source *vlc_source, media_file_array_t *new_files, obs_data_array_t *array)
{
	size_t count = obs_data_array_count(array);
	for (size_t i = 0; i < count; i++)
	{
		obs_data_t *item = obs_data_array_item(array, i);
		const char *path = obs_data_get_string(item, "value");
		if (!path || !*path)
		{
			obs_data_release(item);
			continue;
		}
		os_dir_t *dir = os_opendir(path);

		if (dir)
		{
			struct dstr dir_path = {0};
			struct os_dirent *ent;

			for (;;)
			{
				const char *ext;

				ent = os_readdir(dir);
				if (!ent)
					break;
				if (ent->directory)
					continue;

				ext = os_get_path_extension(ent->d_name);
				if (!valid_extension(ext))
					continue;

				dstr_copy(&dir_path, path);
				dstr_cat_ch(&dir_path, '/');
				dstr_cat(&dir_path, ent->d_name);
				add_file(vlc_source, new_files, dir_path.array);
			}

			dstr_free(&dir_path);
			os_closedir(dir);
		}
		else
		{
			add_file(vlc_source, new_files, path);
		}

		obs_data_release(item);
	}
	obs_data_array_release(array);
}

static bool is_streamlink_config_changed(struct streamlink_config *old, struct streamlink_config *new)
{
	if (old->show_cmd != new->show_cmd || old->enable != new->enable || old->twitch_disable_ads != new->twitch_disable_ads || old->twitch_low_latency_enable != new->twitch_low_latency_enable || strcmp(old->url, new->url) != 0 || strcmp(old->quality, new->quality) != 0 || strcmp(old->rest_options, new->rest_options) != 0)
	{
		return true;
	}
	return false;
}

static bool is_vlc_config_changed(struct vlc_config *old, struct vlc_config *new)
{
	if (old->network_caching != new->network_caching || old->skip_b_frames != new->skip_b_frames || old->subtitle_enable != new->subtitle_enable || old->subtitle_index != new->subtitle_index || old->track_index != new->track_index || strcmp(old->hw_value, new->hw_value) != 0 || strcmp(old->rest_options, new->rest_options) != 0)
	{
		return true;
	}
	return false;
}

static void fill_vlc_config(struct vlc_config *vlc_config, obs_data_t *settings)
{
	vlc_config->network_caching = (int)obs_data_get_int(settings, S_NETWORK_CACHING);
	vlc_config->track_index = (int)obs_data_get_int(settings, S_TRACK);
	vlc_config->subtitle_index = (int)obs_data_get_int(settings, S_SUBTITLE_TRACK);
	vlc_config->subtitle_enable = obs_data_get_bool(settings, S_SUBTITLE_ENABLE);
	vlc_config->skip_b_frames = obs_data_get_bool(settings, S_VLC_SKIP_B_FRAMES);
	vlc_config->hw_value = strdup(obs_data_get_string(settings, S_VLC_HW));
	vlc_config->rest_options = strdup(obs_data_get_string(settings, S_VLC_REST_OPTIONS));
}

static void fill_streamlink_config(struct streamlink_config *streamlink_config, obs_data_t *settings)
{
	streamlink_config->enable = obs_data_get_bool(settings, S_SL_ENABLE);
	streamlink_config->twitch_low_latency_enable = obs_data_get_bool(settings, S_SL_TWITCH_LOW_LATENCY);
	streamlink_config->twitch_disable_ads = obs_data_get_bool(settings, S_SL_TWITCH_DISABLE_ADS);
	streamlink_config->show_cmd = obs_data_get_bool(settings, S_SL_SHOW_CMD);
	streamlink_config->quality = strdup(obs_data_get_string(settings, S_SLQ));
	streamlink_config->url = strdup(obs_data_get_string(settings, S_SL_URL));
	streamlink_config->rest_options = strdup(obs_data_get_string(settings, S_SL_REST_OPTIONS));
}

static void shuffle_playlist(media_file_array_t *files, bool shuffle)
{
	if (files->num > 1 && shuffle)
	{
		media_file_array_t new_files;
		DARRAY(size_t)
		idxs;

		da_init(new_files);
		da_init(idxs);
		da_resize(idxs, files->num);
		da_reserve(new_files, files->num);

		for (size_t i = 0; i < files->num; i++)
		{
			idxs.array[i] = i;
		}
		for (size_t i = idxs.num; i > 0; i--)
		{
			size_t val = rand() % i;
			size_t idx = idxs.array[val];
			da_push_back(new_files, &files->array[idx]);
			da_erase(idxs, val);
		}

		da_free(*files);
		da_free(idxs);
		files = &new_files;
	}
}

static void restart_playback(struct vlc_source *vlc_source)
{
	libvlc_media_list_t *media_list = libvlc_media_list_new_(libvlc);
	libvlc_media_list_lock_(media_list);
	for (size_t i = 0; i < vlc_source->files.num; i++)
	{
		libvlc_media_list_add_media_(media_list, vlc_source->files.array[i].media);
	}
	libvlc_media_list_unlock_(media_list);
	libvlc_media_list_player_set_media_list_(vlc_source->media_list_player, media_list);
	libvlc_media_list_release_(media_list);

	libvlc_media_list_player_set_playback_mode_(vlc_source->media_list_player, vlc_source->loop ? libvlc_playback_mode_loop : libvlc_playback_mode_default);

	if (vlc_source->files.num && (vlc_source->behavior == BEHAVIOR_ALWAYS_PLAY || obs_source_active(vlc_source->source)))
	{
		libvlc_media_list_player_play_(vlc_source->media_list_player);
	}
	else
	{
		obs_source_output_video(vlc_source->source, NULL);
	}
}

static void vlcs_update(void *data, obs_data_t *settings)
{
	media_file_array_t new_files;
	media_file_array_t old_files;
	struct vlc_source *vlc_source = data;
	struct vlc_config vlc_config = {0};
	struct streamlink_config streamlink_config = {0};
	fill_vlc_config(&vlc_config, settings);
	fill_streamlink_config(&streamlink_config, settings);

	// Не проверяет значение monitoring_type потому что на его изменение реагирует функция audio_monitoring_changed.
	bool is_config_changed = is_vlc_config_changed(&vlc_source->vlc_config, &vlc_config)
				|| is_streamlink_config_changed(&vlc_source->streamlink_config, &streamlink_config);
	if (is_config_changed)
	{
		da_init(new_files);
		da_init(old_files);

		replace_vlc_config(&vlc_source->vlc_config, &vlc_config);
		replace_streamling_config(&vlc_source->streamlink_config, &streamlink_config);

		if (strlen(streamlink_config.url) != 0)
		{
			add_file(vlc_source, &new_files, streamlink_config.url);
		}
		else
		{
			add_file_array(vlc_source, &new_files, obs_data_get_array(settings, S_PLAYLIST));
		}

		libvlc_media_list_player_stop_(vlc_source->media_list_player);
		pthread_mutex_lock(&vlc_source->mutex);
		old_files = vlc_source->files;
		vlc_source->files = new_files;
		pthread_mutex_unlock(&vlc_source->mutex);

		terminate_process_array(&old_files);
		free_files(&old_files);
	}

	vlc_source->loop = obs_data_get_bool(settings, S_LOOP);
	vlc_source->shuffle = obs_data_get_int(settings, S_SHUFFLE);
	vlc_source->behavior = (int)obs_data_get_int(settings, S_BEHAVIOR);
	shuffle_playlist(&vlc_source->files, vlc_source->shuffle);
	restart_playback(vlc_source);
}

static void vlcs_started(const struct libvlc_event_t *event, void *data)
{
	struct vlc_source *c = data;
	obs_source_media_started(c->source);

	UNUSED_PARAMETER(event);
}

static void vlcs_stopped(const struct libvlc_event_t *event, void *data)
{
	struct vlc_source *c = data;
	if (!c->loop)
	{
		obs_source_output_video(c->source, NULL);
	}

	obs_source_media_ended(c->source);

	UNUSED_PARAMETER(event);
}

static enum obs_media_state vlcs_get_state(void *data)
{
	struct vlc_source *c = data;
	libvlc_state_t state = libvlc_media_player_get_state_(c->media_player);

	switch (state)
	{
	case libvlc_NothingSpecial:
		return OBS_MEDIA_STATE_NONE;
	case libvlc_Opening:
		return OBS_MEDIA_STATE_OPENING;
	case libvlc_Buffering:
		return OBS_MEDIA_STATE_BUFFERING;
	case libvlc_Playing:
		return OBS_MEDIA_STATE_PLAYING;
	case libvlc_Paused:
		return OBS_MEDIA_STATE_PAUSED;
	case libvlc_Stopped:
		return OBS_MEDIA_STATE_STOPPED;
	case libvlc_Ended:
		return OBS_MEDIA_STATE_ENDED;
	case libvlc_Error:
		return OBS_MEDIA_STATE_ERROR;
	}
	return 0;
}

static void vlcs_play_pause(void *data, bool pause)
{
	struct vlc_source *c = data;
	libvlc_state_t state = libvlc_media_player_get_state_(c->media_player);
	if (pause && state == libvlc_Playing)
	{
		libvlc_media_list_player_pause_(c->media_list_player);
	}
	else if (!pause && state == libvlc_Paused)
	{
		libvlc_media_list_player_play_(c->media_list_player);
	}
}

static void vlcs_restart(void *data)
{
	struct vlc_source *c = data;
	libvlc_media_list_player_stop_(c->media_list_player);
	libvlc_media_list_player_play_(c->media_list_player);
}

static void vlcs_stop(void *data)
{
	struct vlc_source *c = data;
	libvlc_media_list_player_stop_(c->media_list_player);
	obs_source_output_video(c->source, NULL);
}

static void vlcs_playlist_next(void *data)
{
	struct vlc_source *c = data;
	libvlc_media_list_player_next_(c->media_list_player);
}

static void vlcs_playlist_prev(void *data)
{
	struct vlc_source *c = data;
	libvlc_media_list_player_previous_(c->media_list_player);
}

static int64_t vlcs_get_duration(void *data)
{
	struct vlc_source *c = data;
	return (int64_t)libvlc_media_player_get_length_(c->media_player);
}

static int64_t vlcs_get_time(void *data)
{
	struct vlc_source *c = data;
	return (int64_t)libvlc_media_player_get_time_(c->media_player);
}

static void vlcs_set_time(void *data, int64_t ms)
{
	struct vlc_source *c = data;
	libvlc_media_player_set_time_(c->media_player, (libvlc_time_t)ms);
}

static void vlcs_play_pause_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct vlc_source *c = data;
	enum obs_media_state state = obs_source_media_get_state(c->source);

	if (pressed && obs_source_showing(c->source))
	{
		if (state == OBS_MEDIA_STATE_PLAYING)
		{
			obs_source_media_play_pause(c->source, true);
		}
		else if (state == OBS_MEDIA_STATE_PAUSED)
		{
			obs_source_media_play_pause(c->source, false);
		}
	}
}

static void vlcs_restart_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct vlc_source *c = data;
	if (pressed && obs_source_showing(c->source))
	{
		obs_source_media_restart(c->source);
	}
}

static void vlcs_stop_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct vlc_source *c = data;
	if (pressed && obs_source_showing(c->source))
	{
		obs_source_media_stop(c->source);
	}
}

static void vlcs_playlist_next_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct vlc_source *c = data;
	if (pressed && obs_source_showing(c->source))
	{
		obs_source_media_next(c->source);
	}
}

static void vlcs_playlist_prev_hotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	struct vlc_source *c = data;
	if (pressed && obs_source_showing(c->source))
	{
		obs_source_media_previous(c->source);
	}
}

static void *vlcs_create(obs_data_t *settings, obs_source_t *source)
{
	struct vlc_source *c = bzalloc(sizeof(*c));
	c->source = source;
	c->play_pause_hotkey = obs_hotkey_register_source(source, "VLCSource.PlayPause", obs_module_text("PlayPause"), vlcs_play_pause_hotkey, c);
	c->restart_hotkey = obs_hotkey_register_source(source, "VLCSource.Restart", obs_module_text("Restart"), vlcs_restart_hotkey, c);
	c->stop_hotkey = obs_hotkey_register_source(source, "VLCSource.Stop", obs_module_text("Stop"), vlcs_stop_hotkey, c);
	c->playlist_next_hotkey = obs_hotkey_register_source(source, "VLCSource.PlaylistNext", obs_module_text("PlaylistNext"), vlcs_playlist_next_hotkey, c);
	c->playlist_prev_hotkey = obs_hotkey_register_source(source, "VLCSource.PlaylistPrev", obs_module_text("PlaylistPrev"), vlcs_playlist_prev_hotkey, c);

	pthread_mutex_init_value(&c->mutex);
	if (pthread_mutex_init(&c->mutex, NULL) != 0)
		goto error;

	if (!load_libvlc())
		goto error;

	c->media_list_player = libvlc_media_list_player_new_(libvlc);
	if (!c->media_list_player)
		goto error;

	c->media_player = libvlc_media_player_new_(libvlc);
	if (!c->media_player)
		goto error;

	libvlc_media_list_player_set_media_player_(c->media_list_player, c->media_player);

	libvlc_video_set_callbacks_(c->media_player, vlcs_video_lock, NULL, vlcs_video_display, c);
	libvlc_video_set_format_callbacks_(c->media_player, vlcs_video_format, NULL);

	libvlc_audio_set_callbacks_(c->media_player, vlcs_audio_play, NULL, NULL, NULL, NULL, c);
	libvlc_audio_set_format_callbacks_(c->media_player, vlcs_audio_setup, NULL);

	libvlc_event_manager_t *event_manager;
	event_manager = libvlc_media_player_event_manager_(c->media_player);
	libvlc_event_attach_(event_manager, libvlc_MediaPlayerEndReached, vlcs_stopped, c);
	libvlc_event_attach_(event_manager, libvlc_MediaPlayerOpening, vlcs_started, c);

	proc_handler_t *ph = obs_source_get_proc_handler(source);
	proc_handler_add(ph, "void get_metadata(in string tag_id out string tag_data)", vlcs_get_metadata, c);

	obs_source_update(source, NULL);

	UNUSED_PARAMETER(settings);

	obs_source_set_monitoring_type(source, OBS_MONITORING_TYPE_MONITOR_ONLY);

	return c;

error:
	vlcs_destroy(c);
	return NULL;
}

static void vlcs_activate(void *data)
{
	struct vlc_source *c = data;

	if (c->behavior == BEHAVIOR_STOP_RESTART)
	{
		libvlc_media_list_player_play_(c->media_list_player);
	}
	else if (c->behavior == BEHAVIOR_PAUSE_UNPAUSE)
	{
		libvlc_media_list_player_play_(c->media_list_player);
	}
}

static void vlcs_deactivate(void *data)
{
	struct vlc_source *c = data;

	if (c->behavior == BEHAVIOR_STOP_RESTART)
	{
		libvlc_media_list_player_stop_(c->media_list_player);
		obs_source_output_video(c->source, NULL);
	}
	else if (c->behavior == BEHAVIOR_PAUSE_UNPAUSE)
	{
		libvlc_media_list_player_pause_(c->media_list_player);
	}
}

static void vlcs_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, S_LOOP, true);
	obs_data_set_default_bool(settings, S_SHUFFLE, false);
	obs_data_set_default_int(settings, S_BEHAVIOR, BEHAVIOR_STOP_RESTART);
	obs_data_set_default_int(settings, S_NETWORK_CACHING, 400);
	obs_data_set_default_int(settings, S_TRACK, 1);
	obs_data_set_default_bool(settings, S_SUBTITLE_ENABLE, false);
	obs_data_set_default_int(settings, S_SUBTITLE_TRACK, 1);
}

static void missing_file_callback(void *src, const char *new_path, void *data)
{
	struct vlc_source *s = src;
	const char *orig_path = data;

	obs_source_t *source = s->source;
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_array_t *files = obs_data_get_array(settings, S_PLAYLIST);

	size_t l = obs_data_array_count(files);
	for (size_t i = 0; i < l; i++)
	{
		obs_data_t *file = obs_data_array_item(files, i);
		const char *path = obs_data_get_string(file, "value");

		if (strcmp(path, orig_path) == 0)
		{
			if (new_path && *new_path)
				obs_data_set_string(file, "value", new_path);
			else
				obs_data_array_erase(files, i);

			obs_data_release(file);
			break;
		}

		obs_data_release(file);
	}

	obs_source_update(source, settings);

	obs_data_array_release(files);
	obs_data_release(settings);
}

static obs_missing_files_t *vlcs_missingfiles(void *data)
{
	struct vlc_source *s = data;
	obs_missing_files_t *missing_files = obs_missing_files_create();

	obs_source_t *source = s->source;
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_array_t *files = obs_data_get_array(settings, S_PLAYLIST);

	size_t l = obs_data_array_count(files);
	for (size_t i = 0; i < l; i++)
	{
		obs_data_t *item = obs_data_array_item(files, i);
		const char *path = obs_data_get_string(item, "value");

		if (strcmp(path, "") != 0)
		{
			if (!os_file_exists(path) &&
				strstr(path, "://") == NULL)
			{
				obs_missing_file_t *file =
					obs_missing_file_create(
						path, missing_file_callback,
						OBS_MISSING_FILE_SOURCE, source,
						(void *)path);

				obs_missing_files_add_file(missing_files, file);
			}
		}

		obs_data_release(item);
	}

	obs_data_array_release(files);
	obs_data_release(settings);

	return missing_files;
}

