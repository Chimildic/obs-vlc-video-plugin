#include "core.c"

static const char *vlcs_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VLCSource");
}


static obs_properties_t *vlcs_properties(void *data)
{
	obs_properties_t *root_ppts = obs_properties_create();
	struct vlc_source *c = data;
	struct dstr filter = {0};
	struct dstr exts = {0};
	struct dstr path = {0};
	obs_property_t *p;

	obs_properties_set_flags(root_ppts, OBS_PROPERTIES_DEFER_UPDATE);

	if (c)
	{
		pthread_mutex_lock(&c->mutex);
		if (c->files.num)
		{
			struct media_file_data *last = da_end(c->files);
			const char *slash;

			dstr_copy(&path, last->path);
			dstr_replace(&path, "\\", "/");
			slash = strrchr(path.array, '/');
			if (slash)
				dstr_resize(&path, slash - path.array + 1);
		}
		obs_data_t *settings = obs_source_get_settings(c->source);
		obs_data_set_int(settings, S_AM, obs_source_get_monitoring_type(c->source));
		pthread_mutex_unlock(&c->mutex);
	}

	p = obs_properties_add_list(root_ppts, S_AM, T_AM, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, T_AM_NONE, OBS_MONITORING_TYPE_NONE);
	obs_property_list_add_int(p, T_AM_MONITOR_ONLY, OBS_MONITORING_TYPE_MONITOR_ONLY);
	obs_property_list_add_int(p, T_AM_BOTH, OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);
	obs_property_set_modified_callback2(p, audio_monitoring_changed, c);

	p = obs_properties_add_list(root_ppts, S_BEHAVIOR, T_BEHAVIOR, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, T_BEHAVIOR_STOP_RESTART, BEHAVIOR_STOP_RESTART);
	obs_property_list_add_int(p, T_BEHAVIOR_PAUSE_UNPAUSE, BEHAVIOR_PAUSE_UNPAUSE);
	obs_property_list_add_int(p, T_BEHAVIOR_ALWAYS_PLAY, BEHAVIOR_ALWAYS_PLAY);

	dstr_cat(&filter, "Media Files (");
	dstr_copy(&exts, EXTENSIONS_MEDIA);
	dstr_replace(&exts, ";", " ");
	dstr_cat_dstr(&filter, &exts);

	dstr_cat(&filter, ");;Video Files (");
	dstr_copy(&exts, EXTENSIONS_VIDEO);
	dstr_replace(&exts, ";", " ");
	dstr_cat_dstr(&filter, &exts);

	dstr_cat(&filter, ");;Audio Files (");
	dstr_copy(&exts, EXTENSIONS_AUDIO);
	dstr_replace(&exts, ";", " ");
	dstr_cat_dstr(&filter, &exts);

	dstr_cat(&filter, ");;Playlist Files (");
	dstr_copy(&exts, EXTENSIONS_PLAYLIST);
	dstr_replace(&exts, ";", " ");
	dstr_cat_dstr(&filter, &exts);
	dstr_cat(&filter, ")");

	obs_properties_add_editable_list(root_ppts, S_PLAYLIST, T_PLAYLIST, OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS, filter.array, path.array);
	dstr_free(&path);
	dstr_free(&filter);
	dstr_free(&exts);

	obs_properties_add_bool(root_ppts, S_LOOP, T_LOOP);
	obs_properties_add_bool(root_ppts, S_SHUFFLE, T_SHUFFLE);

	p = obs_properties_add_list(root_ppts, S_VLC_HW, T_VLC_HW, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(p, T_VLC_HW_ANY, S_VLC_HW_ANY);
	obs_property_list_add_string(p, T_VLC_HW_DXVA2, S_VLC_HW_DXVA2);
	obs_property_list_add_string(p, T_VLC_HW_D3D11, S_VLC_HW_D3D11);
	obs_property_list_add_string(p, T_VLC_HW_NONE, S_VLC_HW_NONE);

	p = obs_properties_add_bool(root_ppts, S_VLC_SKIP_B_FRAMES, T_VLC_SKIP_B_FRAMES);
    obs_property_set_long_description(p, T_VLC_SKIP_B_FRAMES_DESCRIPTION);

	p = obs_properties_add_int(root_ppts, S_NETWORK_CACHING, T_NETWORK_CACHING, 100, 60000, 10);
	obs_property_int_set_suffix(p, " ms");

	obs_properties_add_int(root_ppts, S_TRACK, T_TRACK, 1, 10, 1);
	obs_properties_add_bool(root_ppts, S_SUBTITLE_ENABLE, T_SUBTITLE_ENABLE);
	obs_properties_add_int(root_ppts, S_SUBTITLE_TRACK, T_SUBTITLE_TRACK, 1, 1000, 1);

	p = obs_properties_add_text(root_ppts, S_VLC_REST_OPTIONS, T_VLC_REST_OPTIONS, OBS_TEXT_DEFAULT);
	obs_property_set_long_description(p, T_VLC_REST_OPTIONS_DESCRIPTION);

	obs_properties_add_button(root_ppts, S_GITHUB_BUTTON, T_GITHUB_BUTTON, github_button_clicked);

	return root_ppts;
}


struct obs_source_info vlc_source_info = {
	.id = "vlc_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO
		| OBS_SOURCE_DO_NOT_DUPLICATE | OBS_SOURCE_CONTROLLABLE_MEDIA,
	.get_name = vlcs_get_name,
	.create = vlcs_create,
	.destroy = vlcs_destroy,
	.update = vlcs_update,
	.get_defaults = vlcs_defaults,
	.get_properties = vlcs_properties,
	.activate = vlcs_activate,
	.deactivate = vlcs_deactivate,
	.missing_files = vlcs_missingfiles,
	.icon_type = OBS_ICON_TYPE_MEDIA,
	.media_play_pause = vlcs_play_pause,
	.media_restart = vlcs_restart,
	.media_stop = vlcs_stop,
	.media_next = vlcs_playlist_next,
	.media_previous = vlcs_playlist_prev,
	.media_get_duration = vlcs_get_duration,
	.media_get_time = vlcs_get_time,
	.media_set_time = vlcs_set_time,
	.media_get_state = vlcs_get_state,
};