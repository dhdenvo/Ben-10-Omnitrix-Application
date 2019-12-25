#include "bentenomnitrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <player.h>
#define PATH_MAX 80
#define ROT_MAX 2

//Linked list to manage the image viewing system
typedef struct loop_image {
	Evas_Object* image;
	struct loop_image* next;
	struct loop_image* prev;
	char* path;
	int show;
	int animated;
	int id;
} loop;

//Structure for controlling an audio clip
typedef struct aud_player {
	player_h player;
	char* path;
} walkman;

//Structure for the data that gets passed throughout the Tizen app
typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *image;
	loop* start_screen;
	loop* image_loop;
	Ecore_Timer* timer;
	walkman* zoom;
	int aud_size;
	int loop_size;
	int rotat;
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

//Method that gets the absolute path of a variable in the res folder
static void
_file_abs_resource_path_get(char *res_file_path, char *abs_path, int buf_size) {
   char *res_dir_path = app_get_resource_path();

   if (res_dir_path)
     {
        snprintf(abs_path, buf_size, "%s%s", res_dir_path, res_file_path);
        free(res_dir_path);
     }
}

//Display all the shown image in an image loop
static void
_display_images(loop* image_loop, Evas_Object* conform, int size) {
	//Loop through the image loop linked list
	for (int i = 0; i < size; i++) {
		//If the current image is visible
		if (image_loop->show == 1) {
			//Tizen image setup (required for all images)
			image_loop->image = elm_image_add(conform);
			elm_image_file_set(image_loop->image, image_loop->path, NULL);
			//If the image is an animated image (animated gif for ex)
			if (image_loop->animated) {
			    elm_image_animated_set(image_loop->image, EINA_TRUE);
			    elm_image_animated_play_set(image_loop->image, EINA_TRUE);
			}
			elm_object_content_set(conform, image_loop->image);
			//Show the image on the gui
			evas_object_show(image_loop->image);
		} else {
			//Hide the image from the gui
			evas_object_hide(image_loop->image);
		}
		//Look at the next image in the linked list
		image_loop = image_loop->next;
	}

}

//Make the next image in the linked list visible
static void
_go_to_next(loop* image_loop, int size) {
	int prev = 0;
	//Loop through the image loop linked list
	for (int i = 0; i < size; i++) {
		//Swap the current image's show value with the previous image's shown value
		prev = prev + image_loop->show;
		image_loop->show = prev - image_loop->show;
		prev = prev - image_loop->show;
		//If the image is the last in the loop and the next shown value should be 1 (on)
		if (i == size - 1 && prev == 1) image_loop->next->show = 1;
		//Look at the next image in the linked list
		image_loop = image_loop->next;
	}
}

//Make the previous image in the linked list visible
static void
_go_to_prev(loop* image_loop, int size) {
	int next = 0;
	//Loop through the image loop linked list
	for (int i = 0; i < size; i++) {
		//Swap the current image's show value with the next image's shown value
		next = next + image_loop->show;
		image_loop->show = next - image_loop->show;
		next = next - image_loop->show;
		//If the image is the last in the loop and the next shown value should be 1 (on)
		if (i == size - 1 && next == 1) image_loop->prev->show = 1;
		//If the image does not have a previous image, use the next one
		if (image_loop->prev == NULL) {
			image_loop = image_loop->next;
		} else {
			image_loop = image_loop->prev;
		}
	}
}

//The method for handling rotation of the watch bezel
Eina_Bool
_rotary_handler_cb(void *data, Eext_Rotary_Event_Info *ev) {
	appdata_s *ad = data;
	//If the startup image isn't being shown run the rotary options
	if (ad->image_loop->show == 0) {
		//If the bezel is rotated clockwise, add to the rotation value
		if (ev->direction == EEXT_ROTARY_DIRECTION_CLOCKWISE) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Rotary device rotated in clockwise direction");
			ad->rotat += 1;
		//If the bezel is rotated counter-clockwise, subtract from the rotation value
		} else {
			dlog_print(DLOG_DEBUG, LOG_TAG, "Rotary device rotated in counter-clockwise direction");
			ad->rotat -= 1;
		}
	}

	//Variable to check if the loop was changed
	int loop_step = 0;
	//If the bezel was rotated counter-clockwise past a certain point, go to the previous image in the loop
    if (ad->rotat == -ROT_MAX) {
    	_go_to_prev(ad->image_loop, ad->loop_size);
    	_display_images(ad->image_loop, ad->conform, ad->loop_size);
    	dlog_print(DLOG_DEBUG, LOG_TAG, "Bezel Prev");
    	//Reset the rotation value
    	ad->rotat = ROT_MAX - 1;
    	loop_step = 1;
	//If the bezel was rotated counter-clockwise past a certain point, go to the next image in the loop
    } else if (ad->rotat == ROT_MAX) {
    	_go_to_next(ad->image_loop, ad->loop_size);
    	_display_images(ad->image_loop, ad->conform, ad->loop_size);
    	dlog_print(DLOG_DEBUG, LOG_TAG, "Bezel Next");
    	//Reset the rotation value
    	ad->rotat = - ROT_MAX + 1;
    	loop_step = 1;
    }

    //If the image was changed, play a swapping audio clip
    if (loop_step) {
    	//Stop all of the audio clips from playing
    	for (int aud_clip = 0; aud_clip < ad->aud_size; aud_clip++)
    		player_stop(ad->zoom[aud_clip].player);
    	//Choose a random swap audio clip to play
    	int rand_int = (rand() % 3) + 1;
    	player_start(ad->zoom[rand_int].player);
    }

    return EINA_FALSE;
}

//Method that runs after the startup timer is finished (kills the timer afterwards)
static void
_advance_pic(void* data, double pos) {
	dlog_print(DLOG_DEBUG, "TIMER STUFF", "Timer Add!");
	appdata_s* ad = data;

	//Stop the startup audio clip
	player_stop(ad->zoom[0].player);

	//Show the next image in the image loop (move from startup clip to first image)
	_go_to_next(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
	//Hide the start screen
	if (ad->start_screen->id == 1)
		evas_object_hide(ad->start_screen->image);
	//Kill the timer that initially ran the method
	ecore_timer_del(ad->timer);
}

//Make all the images not visible
static void
_clear_show(loop* image_loop, int size) {
	//Loop through the image loop linked list
	for (int i = 0; i < size; i++) {
		image_loop->show = 0;
		//Look at the next image in the linked list
		image_loop = image_loop->next;
	}
}

//Reset the image loop (making the first startup image be the only image visible)
static void
_reset_show(loop* image_loop, int size) {
	//Loop through the image loop linked list
	for (int i = 0; i < size; i++) {
		if (i == 0) image_loop->show = 1;
		else image_loop->show = 0;
		//Look at the next image in the linked list
		image_loop = image_loop->next;
	}
}

//Fill an integer array with integers 0 to size - 1 in a random order
static void
_generate_random_order(int* rand_order, int size) {
	//Fill the array with integers 0 to size - 1
	for (int i = 0; i < size; i++) rand_order[i] = i;
	//Randomize the list 4 times (arbitrary number)
	for (int z = 0; z < 4; z++) {
		//Loop through the integer array and randomly swap the integers with eachother
		for (int order = 0; order < size; order++) {
			int rand_int = rand() % size;
	        int temp = rand_order[rand_int];
	        rand_order[rand_int] = rand_order[order];
	        rand_order[order] = temp;
		}
	}

	/*//Loop to check if the randomizing worked
	for (int i = 0; i < size; i++) {
		char snum[30];
		snprintf(snum, 30, "RandInt %d: %d", i, rand_order[i]);
		dlog_print(DLOG_DEBUG, "Random Stuffs", snum);
	}*/
}

//Create the image loop by filling it with images, paths, the order, etc
static void
_create_image_loop(appdata_s* ad) {
	//Generate the first main loop
	ad->image_loop = malloc(sizeof(loop));
	loop* prev_loop = NULL;
	loop* last_loop = NULL;
	//Constant value for the size of the linked list
	ad->loop_size = 15;

	//Create the order list and randomize it
	int* loop_order = malloc(sizeof(int) * ad->loop_size);
	loop_order[0] = -1;
	_generate_random_order(&loop_order[1], ad->loop_size - 1);

	//Loop through the linked list, allocate memory for it, fill it with arrays, and assign the next values
	for (int i = ad->loop_size - 1; i >= 0; i--) {
		//Allocate memory for each image loop
		loop* image_loop = malloc(0);
		if (i == 0) {
			image_loop = ad->image_loop;
		} else {
			image_loop = malloc(sizeof(loop));
		}

		//Set basic values for the image loop
		image_loop->animated = 0;
		image_loop->id = i;
		//Allocate memory for the path of the image
		image_loop->path = malloc(sizeof(char) * PATH_MAX);

		//Define the paths for each image
		//The reason it is a large array of if statements with global string values rather than making it dynamic with an array of strings
		//is because the emulator had trouble getting the path without defining the string globally for each path (or else the app would crash)
		if (loop_order[i] == 0)
			_file_abs_resource_path_get("OmnitrixHeatblast.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 1)
			_file_abs_resource_path_get("OmnitrixXLR8.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 2)
			_file_abs_resource_path_get("OmnitrixUpchuck.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 3)
			_file_abs_resource_path_get("OmnitrixBlitzwolfer.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 4)
			_file_abs_resource_path_get("OmnitrixCannonbolt.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 5)
			_file_abs_resource_path_get("OmnitrixDiamondhead.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 6)
			_file_abs_resource_path_get("OmnitrixFourArms.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 7)
			_file_abs_resource_path_get("OmnitrixGhostfreak.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 8)
			_file_abs_resource_path_get("OmnitrixGreyMatter.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 9)
			_file_abs_resource_path_get("OmnitrixRipjaws.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 10)
			_file_abs_resource_path_get("OmnitrixStinkfly.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 11)
			_file_abs_resource_path_get("OmnitrixUpgrade.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 12)
			_file_abs_resource_path_get("OmnitrixWildmutt.png", image_loop->path, PATH_MAX);
		else if (loop_order[i] == 13)
			_file_abs_resource_path_get("OmnitrixWildvine.png", image_loop->path, PATH_MAX);

		//This is a separate if statement to make the above if statement as dynamic as possible
		if (i == ad->loop_size - 1) last_loop = image_loop;

		//Specific cases for the first and second images in the loops
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

	//Free the memory for the random int array after done with it
	free(loop_order);
	prev_loop = NULL;
	last_loop = NULL;
	loop* loops = ad->image_loop;
	//Loop through the linked list in reverse and assign previous values
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

	ad->start_screen = malloc(sizeof(loop));
	ad->start_screen->animated = 0;
	ad->start_screen->path = malloc(sizeof(char) * PATH_MAX);
	_file_abs_resource_path_get("OmnitrixMain.png", ad->start_screen->path, PATH_MAX);
	ad->start_screen->id = 1;
	ad->start_screen->show = 0;

	/*A basic example of how to display an image on the watch
	char* abs_path_to_image = malloc(sizeof(char) * PATH_MAX);
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


//Create the array of audio clips by assigning paths and preparing them
static void
create_base_audio(appdata_s *ad) {
	//Allocate memory for the array
	ad->zoom = malloc(sizeof(walkman) * ad->aud_size);
	//Loop through the array to assign the paths for each audio clip
	for (int audio_clip = 0; audio_clip < ad->aud_size; audio_clip++) {
		//Create the audio player in the array
		player_create(&ad->zoom[audio_clip].player);
		//Define the path for the audio player
		ad->zoom[audio_clip].path = malloc(sizeof(char) * PATH_MAX);
		switch (audio_clip) {
			case 0: _file_abs_resource_path_get("OmnitrixStartup.mp3", ad->zoom[audio_clip].path, PATH_MAX); break;
			case 1: _file_abs_resource_path_get("OmnitrixAlienSwapOne.mp3", ad->zoom[audio_clip].path, PATH_MAX); break;
			case 2: _file_abs_resource_path_get("OmnitrixAlienSwapTwo.mp3", ad->zoom[audio_clip].path, PATH_MAX); break;
			case 3: _file_abs_resource_path_get("OmnitrixAlienSwapThree.mp3", ad->zoom[audio_clip].path, PATH_MAX); break;
		}
		//Prepare the players
		player_set_uri(ad->zoom[audio_clip].player, ad->zoom[audio_clip].path);
		player_prepare(ad->zoom[audio_clip].player);
	}
}


static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	ad->aud_size = 4;
	create_base_audio(ad);
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

	for (int aud_clip = 0; aud_clip < ad->aud_size; aud_clip++)
		player_stop(ad->zoom[aud_clip].player);

	//Stop the startup timer
	ecore_timer_del(ad->timer);
	//Clear the display
	_clear_show(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
	//Display the start screen
	ad->start_screen->show = 1;
	_display_images(ad->start_screen, ad->conform, 1);

}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
	appdata_s *ad = data;
	//Tests to make sure everything is working correctly
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

	//Hide the start screen
	ad->start_screen->show = 0;
	_display_images(ad->start_screen, ad->conform, 1);

	//Set the rotation value to the default 0
	ad->rotat = 0;
	eext_rotary_event_handler_add(_rotary_handler_cb, ad);

	//Play the startup audio
	player_start(ad->zoom[0].player);

	//Restart the image loop and start the timer
	_reset_show(ad->image_loop, ad->loop_size);
	_display_images(ad->image_loop, ad->conform, ad->loop_size);
	ad->timer = ecore_timer_add(2.55, _advance_pic, ad);
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
	appdata_s *ad = data;

	//Free all the audio paths
	for (int aud_clip = 0; aud_clip < ad->aud_size; aud_clip++)
		free(ad->zoom[aud_clip].path);
	//Free the audio player array
	free(ad->zoom);

	//Loop through the image loop and free all the allocated components
	loop* image_loop = ad->image_loop;
	loop* next = NULL;
	for (int i = 0; i < ad->loop_size; i++) {
		next = image_loop->next;
		//Free the path variables
		free(image_loop->path);
		//Free the image loop itself
		free(image_loop);
		image_loop = next;
	}
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

	srand(time(0));

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
