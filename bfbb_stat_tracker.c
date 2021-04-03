
#include <stdbool.h>
#include <assert.h>

#include <stdint.h>
#include <stdio.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include "memory_reader.h"


#if  defined(DOLPHIN)
#include "dolphin_memory_reader.c"
#elif defined(XBOX)
#include "xbox_memory_reader.c"
#else 
#error PLEASE DEFINE A PLATFORM!!!!
#endif



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
} cb_state_machine ;

typedef struct {
    cb_state_machine* machine;
    game_values* gameval;
    game_values* oldgameval;
} idkwhatever;

void cb_state_machine_call_fail_cb(idkwhatever* idk)
{
    
}

void cb_state_machine_update(idkwhatever* idk) {
    
    cb_state_machine* machine = idk->machine;
    game_values* gameval = idk->gameval;

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
                printf("CB Performed.\n");
            }
            if (!gameval->is_bowling)
            {
                machine->in_cb = false;
                machine->state = cb_NoCB;
            }
        } break;
    }
    
    
}

int main(void) {
    
    idkwhatever idk = {0};
    cb_state_machine machine = {0};
    game_values gameval = {0};
    game_values oldgameval = {0};
    memory_reader reader = {0};
    idk.machine = &machine;
    idk.gameval = &gameval;
    idk.oldgameval = &oldgameval;
    //call update 
    
    init_memory_reader(&reader);
    
    if (!reader.is_hooked) {
        printf("Dolphin not hooked!\n");
        return 1;
    } else {
        printf("Dolphin hooked!\n");
    }
    
    while(true) {
        oldgameval = gameval;
        get_game_values(&reader, &gameval);
        
        if (oldgameval.update_dt != gameval.update_dt)
        {
            cb_state_machine_update(&idk);
        }
        Sleep(1);
    }
    
}