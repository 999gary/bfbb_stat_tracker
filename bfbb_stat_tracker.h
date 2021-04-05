#ifndef BFBB_STAT_TRACKER_H
#define BFBB_STAT_TRACKER_H

#define BUTTON_L   64
#define BUTTON_X 1024

typedef struct {
    s32 startframe;
    s32 endframe;
    float speed;
} cb;

typedef struct {
    u64 frame_count;
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

typedef struct {
    cb_state_machine* machine;
    game_values gameval;
    game_values oldgameval;
    memory_reader reader;
    run* runs;
    nk_context *ctx;
    
    bool is_in_cb;
    cb current_cb;
    
    bool is_in_run;
    run current_run;
} bfbb_stat_tracker;

void update_and_render(bfbb_stat_tracker *);

#endif //BFBB_STAT_TRACKER_H
