
#include <stdbool.h>
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif

#include "stretchy_buffer.h"

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))

s32 window_width = 500;
s32 window_height = 800;

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_INCLUDE_FONT_BAKING
#define NK_GDIP_IMPLEMENTATION
#include "nuklear.h"
typedef struct nk_context nk_context;

typedef struct {
    bool is_loading;
    bool is_bowling;
    bool can_cruise_bubble;
    bool can_bubble_bowl;
    s32 frame_oscillator;
    u8 spat_count;
    u32 player_pointer;
    u32 character;
    char level[8];
    float update_dt;
    float bubble_bowl_speed;
    u32 buttons;
} game_values;

#if  defined(DOLPHIN)
#include "dolphin_memory_reader.c"
#elif defined(XBOX)
#include "xbox_memory_reader.c"
#else 
#error PLEASE DEFINE A PLATFORM!!!!
#endif

#include "bfbb_stat_tracker.h"

#if defined(_WIN32)
#include "win32_gdi_renderer.c"
#else
#error UNKNOWN RENDER PLATFORM!!!!
#endif

#include "style.c"
#include "level_names.h"

// TODO: store average and add it incrementally so this isn't O(n)!
float cb_speed_average(bfbb_stat_tracker *tracker) {
    float cb_speed_sum = 0.0f;
    run current_run = tracker->current_run;
    cb *cruise_boosts = current_run.cruise_boosts;
    int count = sb_count(cruise_boosts);
    if (count == 0) return 0.0f;
    for (int i = 0; i < count; ++i) {
        cb_speed_sum += cruise_boosts[i].speed;
    }
    return cb_speed_sum / (float)count;
}

void cb_state_machine_call_fail_cb(bfbb_stat_tracker* idk)
{
    
}


void cb_state_machine_update(bfbb_stat_tracker* idk) {
    
    cb_state_machine *machine = idk->machine;
    game_values *gameval = &idk->gameval;
    
    if (gameval->character != 0 || !gameval->can_bubble_bowl || !gameval->can_cruise_bubble)
        return;
    
    s32 l_button_is_down = (gameval->buttons & BUTTON_L) != 0;
    s32 x_button_is_down = (gameval->buttons & BUTTON_X) != 0;
    s32 l_and_x_buttons_are_down = l_button_is_down && x_button_is_down;
    
    switch(machine->state) {
        case cb_NoCB: {
            if (l_and_x_buttons_are_down) {
                machine->state = cb_FirstPress; 
            } else if (l_button_is_down || x_button_is_down) {
                machine->state = cb_FailedCB;
            }
        } break;
        case cb_FirstPress: {
            if (!l_and_x_buttons_are_down) {
                machine->state = cb_FirstCB; 
            }
        } break;
        case cb_FirstCB: {
            if (l_and_x_buttons_are_down) {
                if (gameval->bubble_bowl_speed <= 1.0f) {
                    return;
                } else if (gameval->is_bowling) {
                    machine->state = cb_SecondPress;
                }
            } else if (l_button_is_down) {
                machine->state = cb_FirstPress;
            } else if (x_button_is_down) {
                machine->state = cb_FailedCB;
            }
        } break;
        case cb_SecondPress: {
            if (!l_and_x_buttons_are_down) {
                machine->state = cb_SecondCB; 
            }
        } break;
        case cb_FailedCB: {
            if (!l_and_x_buttons_are_down) {
                machine->state = cb_NoCB; 
            }
        } break;
        case cb_SecondCB: {
            if (!machine->in_cb) {
                machine->in_cb = true;
            }
            if (!gameval->is_bowling)
            {
                machine->in_cb = false;
                machine->state = cb_NoCB;
            }
        } break;
    }
    
}

void run_update(bfbb_stat_tracker *tracker) {
    tracker->current_run.frame_count++;
}

void start_run(bfbb_stat_tracker *tracker) {
    tracker->current_run.frame_count = 0;
    tracker->current_run.cruise_boosts = NULL;
    tracker->is_in_run = true;
}

void end_run(bfbb_stat_tracker *tracker) {
    tracker->is_in_run = false;
    sb_push(tracker->runs, tracker->current_run);
}

void frame_update(bfbb_stat_tracker* idk) {
    cb_state_machine_update(idk);
    run_update(idk);
}

void nk_label_printf(struct nk_context *ctx, nk_flags align, const char *fmt, ...) {
    static char buffer[2048];
    
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buffer, 2048, fmt, args);
    va_end(args);
    
    nk_label(ctx, buffer, align);
}



void update_and_render(bfbb_stat_tracker *stat_tracker){
    struct nk_context *ctx = stat_tracker->ctx;
    
    stat_tracker->reader.should_hook = true;
    if(!stat_tracker->reader.is_hooked && stat_tracker->reader.should_hook)
    {
        init_memory_reader(&stat_tracker->reader);
        stat_tracker->reader.should_hook = false;
    }
    
    if(stat_tracker->reader.is_hooked)
    {
        stat_tracker->oldgameval = stat_tracker->gameval;
        get_game_values(&stat_tracker->reader, &stat_tracker->gameval);
        
        
        if (stat_tracker->oldgameval.frame_oscillator != stat_tracker->gameval.frame_oscillator)
        {
            frame_update(stat_tracker);
            
            run *current_run = &stat_tracker->current_run;
            cb *current_cb = &stat_tracker->current_cb;
            
            if (stat_tracker->machine->state == cb_SecondCB) {
                if (!stat_tracker->is_in_cb) {
                    stat_tracker->is_in_cb = true;
                    current_cb->speed = stat_tracker->gameval.bubble_bowl_speed;
                    if (stat_tracker->is_in_run) {
                        current_cb->startframe = current_run->frame_count;
                    }
                }
            } else {
                if (stat_tracker->is_in_cb) {
                    stat_tracker->is_in_cb = false;
                    if (stat_tracker->is_in_run) {
                        current_cb->endframe = current_run->frame_count;
                        sb_push(current_run->cruise_boosts, *current_cb);
                    }
                }
            }
        }
    }
    
    
    if(nk_begin(ctx, "Yep", nk_rect(0, 0, window_width, window_height), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_static(ctx, 30, window_width, 1);
        if (nk_button_label(ctx, (stat_tracker->reader.is_hooked)?"Unhook Dolphin":"Hook Dolphin")) {
            stat_tracker->reader.should_hook = true;
        }
        nk_layout_row_static(ctx, 30, window_width, 2);
        if (stat_tracker->is_in_run) {
            if (nk_button_label(ctx, "End Run")) {
                end_run(stat_tracker);
            }
        } else {
            if (nk_button_label(ctx, "Start Run")) {
                start_run(stat_tracker);
            }
        }
        nk_layout_row_static(ctx, 30, window_width/2, 2);
        if (stat_tracker->gameval.level[0]) nk_label(ctx, get_level_name(stat_tracker->gameval.level), NK_TEXT_ALIGN_LEFT);
        nk_layout_row_static(ctx, 30, window_width/3, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Buttons: %d", stat_tracker->gameval.buttons);
        nk_layout_row_static(ctx, 30, window_width/3, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed: %g", stat_tracker->gameval.bubble_bowl_speed);
        
        if(stat_tracker->is_in_run) {
            run currentrun = stat_tracker->current_run;
            nk_layout_row_static(ctx, 30, window_width/3, 1);
            nk_label(ctx, "Current Run:", NK_TEXT_ALIGN_LEFT);
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Current Frame: %d", currentrun.frame_count);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(currentrun.cruise_boosts) + stat_tracker->is_in_cb);
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %g", cb_speed_average(stat_tracker));
        } else if (sb_count(stat_tracker->runs)) {
            run last_run = sb_last(stat_tracker->runs);
            nk_layout_row_static(ctx, 30, window_width/3, 1);
            nk_label(ctx, "Last Run:", NK_TEXT_ALIGN_LEFT);
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "End Frame: %d", last_run.frame_count);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(last_run.cruise_boosts));
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %g", cb_speed_average(stat_tracker));
        }
        set_style(ctx, THEME_BOB);
    }
    nk_end(ctx);
}



int WinMain(void) {
    bfbb_stat_tracker stat_tracker = {0};
    cb_state_machine machine = {0};
    game_values gameval = {0};
    game_values oldgameval = {0};
    struct nk_font_atlas atlas;
    memory_reader reader = {0};
    stat_tracker.machine = &machine;
    stat_tracker.gameval = gameval;
    stat_tracker.oldgameval = oldgameval;
    stat_tracker.reader = reader;
    stat_tracker.runs = NULL;
    
    start_nk_loop(&stat_tracker);    
}