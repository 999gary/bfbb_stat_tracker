
#include <stdbool.h>
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 800

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

typedef struct {
    s32 startframe;
    s32 endframe;
    float speed;
} cb;

typedef struct {
    u32 frame;
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

typedef struct {
    bool in_cb;
    cb_state state;
} cb_state_machine;

typedef struct {
    cb_state_machine* machine;
    game_values gameval;
    game_values oldgameval;
    memory_reader reader;
    run* runs;
} idkwhatever;


void cb_state_machine_call_fail_cb(idkwhatever* idk)
{
    
}


void cb_state_machine_update(idkwhatever* idk) {
    
    //cb_state_machine* machine = idk->machine;
    //game_values gameval = idk->gameval;
    
    if (idk->gameval.character != 0 || !idk->gameval.can_bubble_bowl || !idk->gameval.can_cruise_bubble)
        return;
    
    s32 l_button_is_down = (idk->gameval.buttons & BUTTON_L) != 0;
    s32 x_button_is_down = (idk->gameval.buttons & BUTTON_X) != 0;
    s32 l_and_x_buttons_are_down = l_button_is_down && x_button_is_down;
    
    switch(idk->machine->state) {
        case cb_NoCB: {
            if (l_and_x_buttons_are_down) {
                idk->machine->state = cb_FirstPress; 
            } else if (l_button_is_down || x_button_is_down) {
                idk->machine->state = cb_FailedCB;
            }
        } break;
        case cb_FirstPress: {
            if (!l_and_x_buttons_are_down) {
                idk->machine->state = cb_FirstCB; 
            }
        } break;
        case cb_FirstCB: {
            if (l_and_x_buttons_are_down) {
                if (idk->gameval.bubble_bowl_speed <= 1.0f) {
                    return;
                } else if (idk->gameval.is_bowling) {
                    idk->machine->state = cb_SecondPress;
                }
            } else if (l_button_is_down) {
                idk->machine->state = cb_FirstPress;
            } else if (x_button_is_down) {
                idk->machine->state = cb_FailedCB;
            }
        } break;
        case cb_SecondPress: {
            if (!l_and_x_buttons_are_down) {
                idk->machine->state = cb_SecondCB; 
            }
        } break;
        case cb_FailedCB: {
            if (!l_and_x_buttons_are_down) {
                idk->machine->state = cb_NoCB; 
            }
        } break;
        case cb_SecondCB: {
            if (!idk->machine->in_cb) {
                idk->machine->in_cb = true;
                cb newcb = {0};
                newcb.startframe = 0;
                newcb.speed = idk->gameval.bubble_bowl_speed;
                newcb.endframe = 0;
                sb_push(sb_last(idk->runs).cruise_boosts, newcb);
            }
            if (!idk->gameval.is_bowling)
            {
                idk->machine->in_cb = false;
                idk->machine->state = cb_NoCB;
            }
        } break;
    }
    
}

void update_dt_update(idkwhatever* idk)
{
    cb_state_machine_update(idk);
}

void update_everything(struct nk_context* ctx, idkwhatever* idk);

#include "win32_gdi_renderer.c"

void update_everything(struct nk_context* ctx, idkwhatever* idk)
{
    if(!idk->reader.is_hooked && idk->reader.should_hook)
    {
        init_memory_reader(&idk->reader);
        printf("Dolphin hooked!\n");
    }

    if(idk->reader.is_hooked)
    {
        idk->oldgameval = idk->gameval;
        get_game_values(&idk->reader, &idk->gameval);
            
            
        if (idk->oldgameval.update_dt != idk->gameval.update_dt)
        {
            update_dt_update(&idk);
        }
    }
       
    float cb_speed_sum = 0.0f;
    int count = sb_count(sb_last(idk->runs).cruise_boosts);
    for (int i = 0; i < count; ++i) {
        cb_speed_sum += sb_last(idk->runs).cruise_boosts[i].speed;
    }
    float average_cruise_boost_speed = cb_speed_sum / (float)count;
        
    printf("\rlevel = \"%s\" | dt = %f | BB speed = %f | cb count = %d | avg cb speed = %f           ", 
            idk->gameval.level,
            idk->gameval.update_dt,
            idk->gameval.bubble_bowl_speed,
            count,
            average_cruise_boost_speed);

    if(nk_begin(ctx, "Yep", nk_rect(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT), NK_WINDOW_NO_SCROLLBAR))
    {
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH, 1);
        if (nk_button_label(ctx, (idk->reader.is_hooked)?"Unhook Dolphin":"Hook Dolphin")) {
            idk->reader.should_hook = true;
        }
        nk_layout_row_static(ctx, 30, WINDOW_WIDTH/3, 3);
        nk_label(ctx, idk->gameval.level, NK_TEXT_ALIGN_LEFT);
        char thing[4];
        sprintf(thing, "Machine State: %d", idk->machine->state);
        nk_label(ctx, thing, NK_TEXT_ALIGN_LEFT);
        char thing2[4];
        sprintf(thing2, "Buttons: %d", idk->gameval.buttons);
        nk_label(ctx, thing2, NK_TEXT_ALIGN_LEFT);
    }
    nk_end(ctx);
        
    Sleep(1);
    

}



int main(void) {

    idkwhatever idk = {0};
    cb_state_machine machine = {0};
    game_values gameval = {0};
    game_values oldgameval = {0};
    memory_reader reader = {0};
    idk.machine = &machine;
    idk.gameval = gameval;
    idk.oldgameval = oldgameval;
    idk.reader = reader;
    idk.runs = NULL;
    run newrun = {0};
    newrun.frame = 0;
    newrun.cruise_boosts = NULL;
    sb_push(idk.runs, newrun);
    

    start_nk_loop(&idk);    
}