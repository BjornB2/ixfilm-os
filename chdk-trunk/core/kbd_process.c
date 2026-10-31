#include "camera.h"
#include "conf.h"
#include "keyboard.h"
#include "action_stack.h"
#include "lang.h"
#include "gui.h"
#include "gui_lang.h"
#include "console.h"
#include "usb_remote.h"
#include "clock.h"
#include "debug_led.h"
#include "shooting.h"
#include "ixfilm_status.h"

#ifdef CAM_IXFILM_OS
extern int module_run(char *name);
#else
#define ALT_UI_BUTTON conf.alt_mode_button
#endif

//-------------------------------------------------------------------
// Keyboard auto repeat

static long last_kbd_key = 0;
static long last_kbd_time = 0;
static long press_count = 0;

long kbd_get_autoclicked_key()
{
	register long key, t;

	key=kbd_get_clicked_key();
	if (key && (key != last_kbd_key))
    {
		last_kbd_key = key;
		press_count = 0;
		last_kbd_time = get_tick_count();
		return key;
	}
    else
    {
		if (last_kbd_key && kbd_is_key_pressed(last_kbd_key))
        {
			t = get_tick_count();
			if (t-last_kbd_time > ((press_count) ? KBD_REPEAT_DELAY : KBD_INITIAL_DELAY))
            {
				++press_count;
				last_kbd_time = t;
				return last_kbd_key;
			}
            else
            {
				return 0;
			}
		}
        else
        {
			last_kbd_key = 0;
			return 0;
		}
	}
}

//-------------------------------------------------------------------

int kbd_blocked;

int kbd_is_blocked()
{
    return kbd_blocked;
}

void enter_alt(int script_mode)
{
    clear_usb_power();         // Prevent previous USB remote pulse from starting script.
    kbd_blocked = 1;
    gui_set_alt_mode_state(script_mode ? ALT_MODE_ENTER_SCRIPT : ALT_MODE_ENTER);
}

void exit_alt()
{
    kbd_blocked = 0;
    gui_set_alt_mode_state(ALT_MODE_LEAVE);
}

//-------------------------------------------------------------------
// Core keyboard handler

long kbd_process()
{
#ifndef CAM_IXFILM_OS
    static int key_pressed;
#else
    /* 0 idle, >0 physical hold ticks, -1 waiting for long-press release,
     * -2 emit the release half of an emulated Canon short press. */
    static int ixfilm_disp_press;
    static unsigned int ixfilm_disp_started;
    static int ixfilm_gui_key_guard;
#endif

    if( usb_HPtimer_handle==0) usb_remote_key();

    if (camera_info.perf.md_af_tuning)
    {
        switch (camera_info.perf.md_af_on_flag)
        {
        case 1:
            if (get_tick_count() >= (int)(camera_info.perf.md_detect_tick + camera_info.perf.md_af_on_delay))
            {
                camera_info.perf.md_af_on_flag = 2;
                camera_set_led(camera_info.cam_af_led,1,200);
            }
            break;
        case 2:
            if (get_tick_count() >= (int)(camera_info.perf.md_detect_tick + camera_info.perf.md_af_on_delay + camera_info.perf.md_af_on_time))
            {
                camera_info.perf.md_af_on_flag = 0;
                camera_set_led(camera_info.cam_af_led,0,0);
            }
            break;
        }
    }

    // check for & process non-keyboard script terminate
    script_check_terminate();

    // Reset keyboard auto repeat if no buttons pressed
    if (kbd_get_pressed_key() == 0)
        last_kbd_key = 0;

    // Set clicked key for scripts.
    if (kbd_get_clicked_key())
    {
        camera_info.state.kbd_last_clicked = kbd_get_clicked_key();
        camera_info.state.kbd_last_clicked_time = get_tick_count();
#ifdef CAM_IXFILM_OS
        ixfilm_screen_activity();
#endif
    }

#ifdef CAM_HAS_JOGDIAL
    // dial movement in script if script is running and set to handle clicks
    if(camera_info.state.state_kbd_script_run && camera_info.state.script_dial_control == DIAL_SCRIPT_KEYCLICK)
    {
        long jog_direction = get_jogdial_direction();
        if (jog_direction)
        {
            camera_info.state.kbd_last_clicked      = jog_direction;
            camera_info.state.kbd_last_clicked_time = get_tick_count();
        }
    }
#endif

    // Set Shutter Half Press state for GUI task.
    camera_info.state.is_shutter_half_press = kbd_is_key_pressed(KEY_SHOOT_HALF);

    // update any state that needs to be updated regularly in shooting
    extern void shooting_update_state(void);
    shooting_update_state();

#ifdef CAM_IXFILM_OS
    /*
     * DISP belongs to Canon on a short press and to IXFilm on a long press.
     * The physical press is hidden until its intent is known. Short presses
     * are replayed to Canon; a hold of roughly 300 ms opens IXFilm once.
     * PLAY is never intercepted in this appliance build.
     */
    if (ixfilm_disp_press == -2)
    {
        kbd_key_press(KEY_DISPLAY);
        ixfilm_disp_press = -3;
        return 1;
    }
    if (ixfilm_disp_press < -2)
    {
        /* Canon's DISP debounce needs more than one 10 ms keyboard scan.
         * Keep the reconstructed short press down for the normal CHDK
         * emulation duration, then emit a clean release. */
        if (ixfilm_disp_press > -(CAM_EMUL_KEYPRESS_DURATION + 2)) {
            kbd_key_press(KEY_DISPLAY);
            ixfilm_disp_press--;
        } else {
            kbd_key_release(KEY_DISPLAY);
            ixfilm_disp_press = 0;
        }
        return 1;
    }
    if (ixfilm_disp_press == -1)
    {
        if (!kbd_is_key_pressed(KEY_DISPLAY)) ixfilm_disp_press = 0;
        kbd_key_release_all();
        return 1;
    }
    if (ixfilm_disp_press > 0)
    {
        if (kbd_is_key_pressed(KEY_DISPLAY))
        {
            if ((unsigned int)(get_tick_count() - ixfilm_disp_started) >= 300)
            {
                module_run("ixfilm.flt");
                ixfilm_disp_press = -1;
            }
            kbd_key_release_all();
            return 1;
        }
        kbd_key_press(KEY_DISPLAY);
        ixfilm_disp_press = -2;
        return 1;
    }
    if (kbd_is_key_pressed(KEY_DISPLAY))
    {
        if (!camera_info.state.gui_mode_none) {
            kbd_key_release_all();
            return 1;
        }
        ixfilm_disp_press = 1;
        ixfilm_disp_started = get_tick_count();
        kbd_key_release_all();
        return 1;
    }
#else

	// Alternative keyboard mode stated/exited by pressing print key.
	// While running Alt. mode shoot key will start a script execution.

	// alt-mode switch and delay emulation
 
	if ( key_pressed  && !usb_remote_active ) 
	{
        if (kbd_is_key_pressed(ALT_UI_BUTTON)
                || ((key_pressed >= CAM_EMUL_KEYPRESS_DELAY)
                && (key_pressed < CAM_EMUL_KEYPRESS_DELAY+CAM_EMUL_KEYPRESS_DURATION)))
        {
            if (key_pressed <= CAM_EMUL_KEYPRESS_DELAY+CAM_EMUL_KEYPRESS_DURATION) 
                key_pressed++;
            if (key_pressed == CAM_EMUL_KEYPRESS_DELAY) 
                kbd_key_press(ALT_UI_BUTTON);
            else if (key_pressed == CAM_EMUL_KEYPRESS_DELAY+CAM_EMUL_KEYPRESS_DURATION) 
                kbd_key_release(ALT_UI_BUTTON);
            return 1;
        } 
        else if (kbd_get_pressed_key() == 0)
        {
            if (key_pressed < CAM_EMUL_KEYPRESS_DELAY)
            {
#ifdef CAM_IXFILM_OS
                /*
                 * IXFilm OS has no public CHDK ALT mode.  A short press of
                 * the configured button opens the film selector directly.
                 * Holding the button still falls through to CHDK's existing
                 * key emulation above, preserving the Canon button action.
                 */
                module_run("ixfilm.flt");
#else
                if (!kbd_blocked)
                {
                    // if start script on alt set, flag to run it
                    if(conf.script_startup==SCRIPT_AUTOSTART_ALT) script_run_on_alt_flag = 1;
                    enter_alt(camera_info.state.state_kbd_script_run);
                }
                else
                    exit_alt();
#endif
            }
            key_pressed = 0;
            return 1;
        }
        return 1;
    }
       
    // auto iso shift
    if (camera_info.state.is_shutter_half_press && kbd_is_key_pressed(ALT_UI_BUTTON))
        return 0;

    if (kbd_is_key_pressed(ALT_UI_BUTTON))
	{
        key_pressed = 1;
        kbd_key_release_all();          
        return 1;
    }
#endif

#ifdef CAM_TOUCHSCREEN_UI
    extern int ts_process_touch();
    if (ts_process_touch())
    {
        gui_set_need_restore();
    }
#endif

    // deals with the rest

    if ( !kbd_blocked || usb_remote_active ) 
	{
		kbd_blocked = handle_usb_remote();
	}

#ifdef CAM_IXFILM_OS
    /* Keep a popup key hidden from Canon until the physical key is released.
     * Without this guard, SET closed IXFilm and the still-held press opened
     * Canon's FUNC/SET menu on the following keyboard scan. */
    if (ixfilm_gui_key_guard)
    {
        if (kbd_get_pressed_key() == 0) ixfilm_gui_key_guard = 0;
        kbd_key_release_all();
        return 1;
    }
    if (gui_kbd_process())
    {
        if (kbd_get_pressed_key() != 0) ixfilm_gui_key_guard = 1;
        return 1;
    }
#else
    if (gui_kbd_process())
        return 1;
#endif

    action_stack_process_all();

    // Check if a PTP script needs to be started
    //  do this after action_stack_process_all so new script is not run until next timeslice
    extern void start_ptp_script();
    start_ptp_script();

    return kbd_blocked;
}
