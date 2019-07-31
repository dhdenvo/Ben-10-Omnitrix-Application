#include "bentenomnitrix.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#define PATH_MAX 80

typedef struct loop_image {
	Evas_Object* image;
	struct loop_image* next;
	struct loop_image* prev;
	char* path;
	int show;
	int animated;
	int id;
} loop;

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *image;
	loop* start_screen;
	loop* image_loop;
	int loop_size;
	int rotat;
	int start;
	Ecore_Timer* timer;
} appdata_s;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void
_file_abs_resource_path_get(char *res_file_path, char *abs_path, int buf_size) {
   char *res_dir_path = app_get_resource_path();

   if (res_dir_path)
     {
        snprintf(abs_path, buf_size, "%s%s", res_dir_path, res_file_path);
        free(res_dir_path);
     }
}

static void
_display_images(loop* image_loop, Evas_Object* conform, int size) {
	for (int i = 0; i < size; i++) {
		if (image_loop->show == 1) {
			image_loop->image = elm_image_add(conform);
			elm_image_file_set(image_loop->image, image_loop->path, NULL);
			if (image_loop->animated) {
			    elm_image_animated_set(image_loop->image, EINA_TRUE);
			    elm_image_animated_play_set(image_loop->image, EINA_TRUE);
			}
			elm_object_content_set(conform, image_loop->image);
			evas_object_show(image_loop->image);
		} else {
			evas_object_hide(image_loop->image);
		}
		image_loop = image_loop->next;
	}

}

static void
_go_to_next(loop* image_loop, int size) {
	int prev = 0;
	for (int i = 0; i < size; i++) {
		prev = prev + image_loop->show;
		image_loop->show = prev - image_loop->show;
		prev = prev - image_loop->show;
		if (i == size - 1 && prev == 1) image_loop->next->show = 1;
		image_loop = image_loop->next;
	}
}


static void
_go_to_prev(loop* image_loop, int size) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "Previous Stuffsssss");
	int next = 0;
	for (int i = 0; i < size; i++) {

		switch (image_loop->id) {
			case 0: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Zero"); break;
			case 1: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Uno"); break;
			case 2: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Due"); break;
			case 3: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Tre"); break;
		}

		next = next + image_loop->show;
		image_loop->show = next - image_loop->show;
		next = next - image_loop->show;
		if (i == size - 1 && next == 1) image_loop->prev->show = 1;
		if (image_loop->prev == NULL) {
			image_loop = image_loop->next;
			dlog_print(DLOG_DEBUG, LOG_TAG, "CRAPPPP AGAIIIIIIIINNNNNNNNNNN");
		} else {
			image_loop = image_loop->prev;
		}
	}
}


Eina_Bool
_rotary_handler_cb(void *data, Eext_Rotary_Event_Info *ev) {
	appdata_s *ad = data;
	if (ad->image_loop->show == 0) {
		if (ev->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Rotary device rotated in clockwise direction");
			ad->rotat += 1;
		} else {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Rotary device rotated in counter-clockwise direction");
			ad->rotat -= 1;
		}
	}

    if (ad->rotat == -8) {
    	_go_to_prev(ad->image_loop, ad->loop_size);
    	_display_images(ad->image_loop, ad->conform, ad->loop_size);
    	dlog_print(DLOG_DEBUG, LOG_TAG, "Bezel Prev");
    	ad->rotat = 7;
    } else if (ad->rotat == 8) {
    	_go_to_next(ad->image_loop, ad->loop_size);
    	_display_images(ad->image_loop, ad->conform, ad->loop_size);
    	dlog_print(DLOG_DEBUG, LOG_TAG, "Bezel Next");
    	ad->rotat = -7;
    }

    return EINA_FALSE;
}


static void
_advance_pic(void* data, double pos) {
	dlog_print(DLOG_DEBUG, "TIMER STUFF", "Timer Add!");
	appdata_s* ad = data;
	_go_to_next(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
	if (ad->start && ad->start_screen->id == 1)
		evas_object_hide(ad->start_screen->image);
	ecore_timer_del(ad->timer);
}

static void
_clear_show(loop* image_loop, int size) {
	for (int i = 0; i < size; i++) {
		image_loop->show = 0;
		image_loop = image_loop->next;
	}
}

static void
_reset_show(loop* image_loop, int size) {
	for (int i = 0; i < size; i++) {
		if (i == 0)
			image_loop->show = 1;
		else
			image_loop->show = 0;
		image_loop = image_loop->next;
	}
}

static void
_create_image_loop(appdata_s* ad) {
	ad->image_loop = malloc(sizeof(loop));
	loop* prev_loop = NULL;
	loop* last_loop = NULL;
	ad->loop_size = 15;

	for (int i = ad->loop_size - 1; i >= 0; i--) {
		loop* image_loop = malloc(0);
		if (i == 0) {
			image_loop = ad->image_loop;
		} else {
			image_loop = malloc(sizeof(loop));
		}

		image_loop->animated = 0;
		image_loop->id = i;
		image_loop->path = malloc(sizeof(char) * PATH_MAX);

		if (i == 1)
			_file_abs_resource_path_get("OmnitrixHeatblast.png", image_loop->path, PATH_MAX);
		else if (i == 2)
			_file_abs_resource_path_get("OmnitrixXLR8.png", image_loop->path, PATH_MAX);
		else if (i == 3)
			_file_abs_resource_path_get("OmnitrixUpchuck.png", image_loop->path, PATH_MAX);
		else if (i == 4)
			_file_abs_resource_path_get("OmnitrixBlitzwolfer.png", image_loop->path, PATH_MAX);
		else if (i == 5)
			_file_abs_resource_path_get("OmnitrixCannonbolt.png", image_loop->path, PATH_MAX);
		else if (i == 6)
			_file_abs_resource_path_get("OmnitrixDiamondhead.png", image_loop->path, PATH_MAX);
		else if (i == 7)
			_file_abs_resource_path_get("OmnitrixFourArms.png", image_loop->path, PATH_MAX);
		else if (i == 8)
			_file_abs_resource_path_get("OmnitrixGhostfreak.png", image_loop->path, PATH_MAX);
		else if (i == 9)
			_file_abs_resource_path_get("OmnitrixGreyMatter.png", image_loop->path, PATH_MAX);
		else if (i == 10)
			_file_abs_resource_path_get("OmnitrixRipjaws.png", image_loop->path, PATH_MAX);
		else if (i == 11)
			_file_abs_resource_path_get("OmnitrixStinkfly.png", image_loop->path, PATH_MAX);
		else if (i == 12)
			_file_abs_resource_path_get("OmnitrixUpgrade.png", image_loop->path, PATH_MAX);
		else if (i == 13)
			_file_abs_resource_path_get("OmnitrixWildmutt.png", image_loop->path, PATH_MAX);
		else if (i == 14)
			_file_abs_resource_path_get("OmnitrixWildvine.png", image_loop->path, PATH_MAX);
		if (i == ad->loop_size - 1)
			last_loop = image_loop;

		/*switch(i){
			case 1:
				_file_abs_resource_path_get("OmnitrixHeatblast.png", image_loop->path, PATH_MAX); break;
			case 2:
				_file_abs_resource_path_get("OmnitrixXLR8.png", image_loop->path, PATH_MAX); break;
			case 3:
				_file_abs_resource_path_get("OmnitrixUpchuck.png", image_loop->path, PATH_MAX); break;
			case 4:
				_file_abs_resource_path_get("OmnitrixBlitzwolfer.png", image_loop->path, PATH_MAX); break;
			case 5:
				_file_abs_resource_path_get("OmnitrixCannonbolt.png", image_loop->path, PATH_MAX); break;
			case 6:
				_file_abs_resource_path_get("OmnitrixDiamondhead.png", image_loop->path, PATH_MAX); break;
			case 7:
				_file_abs_resource_path_get("OmnitrixFourArms.png", image_loop->path, PATH_MAX); break;
			case 8:
				_file_abs_resource_path_get("OmnitrixGhostfreak.png", image_loop->path, PATH_MAX); break;
			case 9:
				_file_abs_resource_path_get("OmnitrixGreyMatter.png", image_loop->path, PATH_MAX); break;
			case 10:
				_file_abs_resource_path_get("OmnitrixRipjaws.png", image_loop->path, PATH_MAX); break;
			case 11:
				_file_abs_resource_path_get("OmnitrixStinkfly.png", image_loop->path, PATH_MAX); break;
			case 12:
				_file_abs_resource_path_get("OmnitrixUpgrade.png", image_loop->path, PATH_MAX); break;
			case 13:
				_file_abs_resource_path_get("OmnitrixWildmutt.png", image_loop->path, PATH_MAX); break;
			case 14:
				_file_abs_resource_path_get("OmnitrixWildvine.png", image_loop->path, PATH_MAX); break;
		}*/

		switch(i) {
			case 0:
				_file_abs_resource_path_get("OmnitrixStartup.gif", image_loop->path, PATH_MAX);
				image_loop->animated = 1;
				break;
			case 1:
				last_loop->next = image_loop; break;
		}

		image_loop->next = prev_loop;
		prev_loop = image_loop;

	}

	prev_loop = NULL;
	last_loop = NULL;
	loop* loops = ad->image_loop;
	for (int i = 0; i < ad->loop_size; i++) {
		loops->prev = prev_loop;
		prev_loop = loops;
		if (i == 1) last_loop = loops;
		if (i == ad->loop_size - 1) last_loop->prev = loops;
		loops = loops->next;
	}

	_reset_show(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	/* Create and initialize elm_win.
	   elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	/* Create and initialize elm_conformant.
	   elm_conformant is mandatory for base gui to have proper size
	   when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);


	/* Create an actual view of the base gui.
	   Modify this part to change the view. */

	ad->rotat = 0;
    eext_rotary_event_handler_add(_rotary_handler_cb, ad);

	_create_image_loop(ad);

	if (ad->start) {
		ad->start_screen = malloc(sizeof(loop));
		ad->start_screen->animated = 0;
		ad->start_screen->path = malloc(sizeof(char) * PATH_MAX);
		_file_abs_resource_path_get("OmnitrixMain.png", ad->start_screen->path, PATH_MAX);
		ad->start_screen->id = 1;
		ad->start_screen->show = 0;
	}

	/*char* abs_path_to_image = malloc(sizeof(char) * PATH_MAX);
	 //_file_abs_resource_path_get("hulk.png", abs_path_to_image, PATH_MAX);
	 _file_abs_resource_path_get("OmnitrixStartup.gif", abs_path_to_image, PATH_MAX);

	ad->image = elm_image_add(ad->conform);
    elm_image_file_set(ad->image, abs_path_to_image, NULL);

    //Animated Image Settings
    elm_image_animated_set(ad->image, EINA_TRUE);
    elm_image_animated_play_set(ad->image, EINA_TRUE);

	elm_object_content_set(ad->conform, ad->image);
	evas_object_show(ad->image);
	free(abs_path_to_image);*/

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	ad->start = 1;
	create_base_gui(ad);
	ecore_animator_frametime_set(1. / 50);

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
	appdata_s *ad = data;
	ecore_timer_del(ad->timer);
	_clear_show(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
	if (ad->start) {
		ad->start_screen->show = 1;
		_display_images(ad->start_screen, ad->conform, 1);
	}

}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
	appdata_s *ad = data;
	/*if (ad->image_loop->next->prev != NULL) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "YAYYYYYYYYYY");
		switch(ad->image_loop->next->prev->id) {
			case 0: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Zero"); break;
			case 1: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Uno"); break;
			case 2: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Due"); break;
			case 3: dlog_print(DLOG_DEBUG, LOG_TAG, "Stuff and Things Tre"); break;
		}
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "CRAPPPPPPPPPPPP!!!!!!");
	}*/

	/*loop* loops = ad->image_loop;
	int next_valid = 1;
	for (int i = 0; i < ad->loop_size + 4; i++) {
		if (loops->next == NULL) {
			next_valid = 0;
		}
	}
	if (next_valid) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "YAYYYYYYYYYY");
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "CRAPPPPPPPPPPPP!!!!!!");
	}*/

	ad->start_screen->show = 0;
	_display_images(ad->start_screen, ad->conform, 1);

	ad->rotat = 0;
	eext_rotary_event_handler_add(_rotary_handler_cb, ad);

	_reset_show(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
	ad->timer = ecore_timer_add(2.5, _advance_pic, ad);
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
