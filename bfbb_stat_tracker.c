
#include <stdbool.h>
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 800


struct nk_context;
typedef struct idkwhatever idkwhatever;

void update_everything(struct nk_context* ctx, idkwhatever* idk);

#include "win32_gdi_renderer.c"
#include "style.c"


#include "stretchy_buffer.h"

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include "memory_reader.h"


//#if defined(WIN32)

//#else
//#error UNKNOWN RENDER PLATFORM!!!!
//#endif


#if  defined(DOLPHIN)
#include "dolphin_memory_reader.c"
#elif defined(XBOX)
#include "xbox_memory_reader.c"
#else 
#error PLEASE DEFINE A PLATFORM!!!!
#endif

#include "level_names.h"

typedef struct {
    s32 startframe;
    s32 endframe;
    float speed;
} cb;

typedef struct {
    u32 frame;
    bool is_done;
    u32 endframe;
    cb *cruise_boosts;
} run;

typedef enum {
    cb_NoCB,
    cb_FailedCB,
    cb_FirstPress,
    cb_FirstCB,
    cb_SecondPress,
    cb_SecondCB
} cb_state;

const char *get_cb_state_string(cb_state state) {
    assert(state >= 0 && state <= cb_SecondCB);
    const char *states[] = {
        "No CB",
        "Failed CB",
        "First Press",
        "First CB",
        "Second Press",
        "Second CB"
    };
    return states[state];
}

typedef struct {
    bool in_cb;
    cb_state state;
} cb_state_machine;

struct idkwhatever {
    cb_state_machine* machine;
    game_values gameval;
    game_values oldgameval;
    memory_reader reader;
    run* runs;
};

float cb_speed_average(idkwhatever* idk) {
    float cb_speed_sum = 0.0f;
    int count = sb_count(sb_last(idk->runs).cruise_boosts);
    for (int i = 0; i < count; ++i) {
        cb_speed_sum += sb_last(idk->runs).cruise_boosts[i].speed;
    }
    return cb_speed_sum / (float)count;
}

void cb_state_machine_call_fail_cb(idkwhatever* idk)
{
    
}


void cb_state_machine_update(idkwhatever* idk) {
    
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
                cb newcb = {0};
                newcb.startframe = 0;
                newcb.speed = gameval->bubble_bowl_speed;
                newcb.endframe = 0;
                sb_push(sb_last(idk->runs).cruise_boosts, newcb);
            }
            if (!gameval->is_bowling)
            {
                machine->in_cb = false;
                machine->state = cb_NoCB;
            }
        } break;
    }
    
}

void run_update(idkwhatever* idk) {
    if(sb_count(idk->runs) >= 1 && !sb_last(idk->runs).is_done)
        sb_last(idk->runs).frame++;
}

void start_run(idkwhatever* idk) {
    run newrun = {0};
    newrun.frame = 0;
    newrun.cruise_boosts = NULL;
    sb_push(idk->runs, newrun);
}

void end_run(idkwhatever* idk) {
    run* currentrun = &sb_last(idk->runs);
    if (!currentrun->is_done) {
        currentrun->is_done = true;
        currentrun->endframe = currentrun->frame;
    }
}

void update(idkwhatever* idk) {
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



void update_everything(struct nk_context* ctx, idkwhatever* idk)
{
    idk->reader.should_hook = true;
    if(!idk->reader.is_hooked && idk->reader.should_hook)
    {
        init_memory_reader(&idk->reader);
        idk->reader.should_hook = false;
    }
    
    if(idk->reader.is_hooked)
    {
        idk->oldgameval = idk->gameval;
        get_game_values(&idk->reader, &idk->gameval);
        
        
        if (idk->oldgameval.update_dt != idk->gameval.update_dt)
        {
            update(idk);
        }
    }

    
    if(nk_begin(ctx, "Yep", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH, 1);
        if (nk_button_label(ctx, (idk->reader.is_hooked)?"Unhook Dolphin":"Hook Dolphin")) {
            idk->reader.should_hook = true;
        }
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH, 2);
        if (sb_count(idk->runs) == 0 || sb_last(idk->runs).is_done) {
            if (nk_button_label(ctx, "Start Run")) {
                start_run(idk);
            }
        } else if(sb_count(idk->runs) >= 1 && !sb_last(idk->runs).is_done) {
            if (nk_button_label(ctx, "End Run")) {
                end_run(idk);
            }
        }
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH/2, 2);
        if (idk->gameval.level[0]) nk_label(ctx, get_level_name(idk->gameval.level), NK_TEXT_ALIGN_LEFT);
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH/3, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Buttons: %d", idk->gameval.buttons);
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH/3, 1);
        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed: %f", idk->gameval.bubble_bowl_speed);
        
        if(sb_count(idk->runs) >= 1 && !sb_last(idk->runs).is_done) {
            run currentrun = sb_last(idk->runs);
            nk_layout_row_static(ctx, 30, WINDOW_WIDTH/3, 1);
            nk_label(ctx, "Current Run:", NK_TEXT_ALIGN_LEFT);
            nk_layout_row_static(ctx, 30, WINDOW_WIDTH/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Current Frame: %d", currentrun.frame);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(currentrun.cruise_boosts));
            nk_layout_row_static(ctx, 30, WINDOW_WIDTH/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %f", cb_speed_average(idk));
        } else if (sb_count(idk->runs) >= 1) {
            run currentrun = sb_last(idk->runs);
            nk_layout_row_static(ctx, 30, WINDOW_WIDTH/3, 1);
            nk_label(ctx, "Last Run:", NK_TEXT_ALIGN_LEFT);
            nk_layout_row_static(ctx, 30, WINDOW_WIDTH/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "End Frame: %d", currentrun.endframe);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(currentrun.cruise_boosts));
            nk_layout_row_static(ctx, 30, WINDOW_WIDTH/2, 2);
            nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %f", cb_speed_average(idk));
        }
        set_style(ctx, THEME_BOB);
    }
    nk_end(ctx);
}



int WinMain(void) {
    idkwhatever idk = {0};
    cb_state_machine machine = {0};
    game_values gameval = {0};
    game_values oldgameval = {0};
    struct nk_font_atlas atlas;
    memory_reader reader = {0};
    idk.machine = &machine;
    idk.gameval = gameval;
    idk.oldgameval = oldgameval;
    idk.reader = reader;
    idk.runs = NULL;
    
    start_nk_loop(&idk);    
}