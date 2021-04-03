
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

    switch(machine->state) {
        case cb_NoCB: {
            if ((gameval->buttons & BUTTON_L) != 0 && (gameval->buttons & BUTTON_L) != 0) {
                machine->state = cb_FirstPress; 
            } else if ((gameval->buttons & BUTTON_L) != 0 || (gameval->buttons & BUTTON_L) != 0) {
                machine->state = cb_FailedCB;
            }
        } break;
        case cb_FirstPress: {
            if ((gameval->buttons & BUTTON_L) == 0 && (gameval->buttons & BUTTON_L) == 0) {
                machine->state = cb_FirstPress; 
            }
        } break;
        case cb_FirstCB: {
            if ((gameval->buttons & BUTTON_L) != 0 && (gameval->buttons & BUTTON_L) != 0) {
                if (gameval->bubble_bowl_speed <= 1.0f) {
                    return;
                } else if (gameval->is_bowling) {
                    machine->state = cb_SecondPress;
                }
            } else if ((gameval->buttons & BUTTON_L) != 0) {

                machine->state = cb_FirstPress;
            } else if ((gameval->buttons & BUTTON_X) != 0) {
                machine->state = cb_FailedCB;
            }
        } break;
        case cb_SecondPress: {
            if ((gameval->buttons & BUTTON_L) == 0 && (gameval->buttons & BUTTON_L) == 0) {
                machine->state = cb_SecondCB; 
            }
        } break;
        case cb_SecondCB: {
            if ((gameval->buttons & BUTTON_L) == 0 && (gameval->buttons & BUTTON_L) == 0) {
                machine->state = cb_SecondCB; 
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
        printf("Dolphin not hooked!");
        return 1;
    } else {
        printf("Dolphin hooked!");
    }

    while(true) {
        
        oldgameval = gameval;
        get_game_values(&reader, &gameval);
        cb_state_machine_update(&idk);

    }

}