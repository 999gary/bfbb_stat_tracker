
#include <stdbool.h>
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
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

typedef union {
    struct bools_dummy_{
        s32 is_jumping;
        s32 is_d_jumping;
        s32 is_bubble_spinning;
        s32 is_bubble_bouncing;
        s32 is_bubble_bashing;
        s32 is_bubble_bowling;
        s32 was_d_jumping;
        s32 is_coptering;
    };
    s32 all[sizeof(struct bools_dummy_)/sizeof(s32)];
} player_bools;

typedef struct {
    player_bools bools;
    bool can_cruise_bubble;
    bool can_bubble_bowl;
    u32 can_jump;
    u32 health;
    s32 frame_oscillator;
    u8 spat_count;
    u32 player_pointer;
    u32 character;
    char level[8];
    float update_dt;
    float bubble_bowl_speed;
    u32 buttons;
    u32 anim_id;
} game_values;

int of_counter = 0;
int of_frameon = 0;
int of_framepress = 0;
bool should_count = false;

#if  defined(DOLPHIN)
#include "dolphin_memory_reader.c"
#elif defined(XBOX)
#include "xbox_memory_reader.c"
#else 
#error PLEASE DEFINE A PLATFORM!!!!
#endif


#include "networking.c"
//#include "bfbb_stat_tracker.h"

#if defined(_WIN32)
#include "win32_gdi_renderer.c"
#else
#error UNKNOWN RENDER PLATFORM!!!!
#endif


#include "style.c"
#include "level_names.h"

void of_state_machine_update(bfbb_stat_tracker* stat_tracker) {
    of_state_machine *of_state_machine = stat_tracker->of_state_machine;
    game_values *gameval = &stat_tracker->gameval;
    
    if (gameval->character != 0)
        return;
    
    if (should_count) {
        of_counter++;
    }

    switch(of_state_machine->state) {
        case of_Undamaged: {
            //TODO: check for damage animation.
            if ((gameval->anim_id == 4189683632 || 
                 gameval->anim_id == 4189683633 || 
                 gameval->anim_id == 4189683634 || 
                 gameval->anim_id == 4189683635 || 
                 gameval->anim_id == 4189683636)) {
                of_state_machine->state = of_DamagedOnFrame;
                of_frameon = 0;
                of_framepress = 0;
                of_counter = 0;
                should_count = true;
            }
        } break;
        case of_DamagedOnFrame: {
            if (gameval->anim_id == 1822369153)
            {
                of_frameon = of_counter;
                of_state_machine->state = of_DamagedPostFrame;
            }
        } break;
        case of_DamagedPostFrame: {
            if (gameval->anim_id != 1164637556){
                of_state_machine->state = of_Undamaged;
                should_count = false;
            }
            
        }
    }

    if ((gameval->buttons & BUTTON_A) != 0 && should_count)
    {
        of_framepress = of_counter;
    }
    
    
}

void start_run(bfbb_stat_tracker *tracker) {
    memset(&tracker->current_run, 0, sizeof(tracker->current_run));
    tracker->is_in_run = true;
}

void end_run(bfbb_stat_tracker *tracker) {
    sb_push(tracker->runs, tracker->current_run);
    tracker->is_in_run = false;
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
    strncpy(&stat_tracker->buffer, "getcurrenttimerphase\r\n", 256);
    
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
        
        run *current_run = &stat_tracker->current_run;
        cb *current_cb = &stat_tracker->current_cb;
        
        game_values *gameval = &stat_tracker->gameval;
        
        if (stat_tracker->oldgameval.frame_oscillator != stat_tracker->gameval.frame_oscillator)
        {
            if(stat_tracker->counter == 60) {
                stat_tracker->counter = 0;
                strncpy(&stat_tracker->oldresponse, stat_tracker->response, 256);
                memset(stat_tracker->response, 0, 256);
                init_client(stat_tracker);
                run_client(stat_tracker);
            }
            if(stat_tracker->oldresponse[0] == 'N' && stat_tracker->response[0] == 'R') {
                start_run(stat_tracker);
            }
            if(stat_tracker->is_in_run && stat_tracker->oldresponse[0] == 'R' && stat_tracker->response[0] == 'N') {
                end_run(stat_tracker);
            }
            if ((gameval->anim_id == 1291389524 || gameval->anim_id == 1679544279) && 
                gameval->bools.is_bubble_bowling &&
                gameval->bubble_bowl_speed >= 1.0f) {
                if (!stat_tracker->is_in_cb) {
                    stat_tracker->is_in_cb = true;
                    float speed = stat_tracker->gameval.bubble_bowl_speed;
                    current_cb->speed = speed;
                    if (stat_tracker->is_in_run) {
                        current_cb->startframe = current_run->frame_count;
                        float av = current_run->cb_average_speed;
                        int count = sb_count(current_run->cruise_boosts);
                        av = (av*count + speed) / (count + 1);
                        current_run->cb_average_speed = av;
                    }
                }
                stat_tracker->is_in_cb = true;
            } else {
                if (stat_tracker->is_in_cb) {
                    stat_tracker->is_in_cb = false;
                    if (stat_tracker->is_in_run) {
                        current_cb->endframe = current_run->frame_count;
                        sb_push(current_run->cruise_boosts, *current_cb);
                    }
                }
                stat_tracker->is_in_cb = false;
            }
            
            
            of_state_machine_update(stat_tracker);
            
            stat_tracker->current_run.frame_count++;
            stat_tracker->counter++;

        }
        /*if (stat_tracker->oldgameval.buttons != stat_tracker->gameval.buttons) {
            cb_state_machine_update(stat_tracker);
        }*/
        
        if (stat_tracker->oldgameval.anim_id != stat_tracker->gameval.anim_id) {
            
        }
        
        game_values *old = &stat_tracker->oldgameval;
        for (s32 i = 0; i < ArrayCount(gameval->bools.all); ++i) {
            if (old->bools.all[i] == 0 && gameval->bools.all[i] != 0) {
                current_run->bool_counts.all[i]++;
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
        nk_layout_row_static(ctx, 30, window_width/3, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "OF State: %d", stat_tracker->of_state_machine->state);
        nk_layout_row_static(ctx, 30, window_width/3, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "OF Missed: %d", of_frameon-of_framepress);
        
        if(stat_tracker->is_in_run) {
            run currentrun = stat_tracker->current_run;
            nk_layout_row_static(ctx, 30, window_width/3, 1);
            nk_label(ctx, "Current Run:", NK_TEXT_ALIGN_LEFT);
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Current Frame: %d", currentrun.frame_count);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(currentrun.cruise_boosts) + stat_tracker->is_in_cb);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %.20f", currentrun.cb_average_speed);
            
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Current CB Speed: %g", stat_tracker->current_cb.speed);
            
            // YEP
            
            player_bools bool_counts = stat_tracker->current_run.bool_counts;
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Jumps: %u", bool_counts.is_jumping);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Double Jumps: %u", bool_counts.is_d_jumping);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Spins: %u", bool_counts.is_bubble_spinning);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Bounces: %u", bool_counts.is_bubble_bouncing);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Bashes: %u", bool_counts.is_bubble_bashing);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Bowls: %u", bool_counts.is_bubble_bowling);
            
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Copters: %u", bool_counts.is_coptering);
            
        } else if (sb_count(stat_tracker->runs)) {
            run last_run = sb_last(stat_tracker->runs);
            nk_layout_row_static(ctx, 30, window_width/3, 1);
            nk_label(ctx, "Last Run:", NK_TEXT_ALIGN_LEFT);
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "End Frame: %d", last_run.frame_count);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(last_run.cruise_boosts));
            nk_layout_row_static(ctx, 30, window_width/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %g", last_run.cb_average_speed);
        }
        nk_layout_row_static(ctx, 30, window_width/2, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Animation ID: %u", stat_tracker->gameval.anim_id);
        set_style(ctx, THEME_BOB);
    }
    nk_end(ctx);


}



int WinMain(void) {
    bfbb_stat_tracker stat_tracker = {0};
    of_state_machine of_state_machine = {0};
    game_values gameval = {0};
    game_values oldgameval = {0};
    struct nk_font_atlas atlas;
    memory_reader reader = {0};
    stat_tracker.of_state_machine = &of_state_machine;
    stat_tracker.gameval = gameval;
    stat_tracker.oldgameval = oldgameval;
    stat_tracker.reader = reader;
    stat_tracker.runs = NULL;
    
    //init_client(&stat_tracker);

    start_nk_loop(&stat_tracker);    
}