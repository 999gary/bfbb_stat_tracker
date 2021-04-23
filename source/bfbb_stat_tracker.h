#ifndef BFBB_STAT_TRACKER_H
#define BFBB_STAT_TRACKER_H

#define BUTTON_L   64
#define BUTTON_A  256
#define BUTTON_X 1024

typedef struct {
    s32 startframe;
    s32 endframe;
    float speed;
} cb;

typedef struct {
    player_bools bool_counts;
    u64 frame_count;
    float cb_average_speed;
    cb *cruise_boosts;
} run;

// NOTE: if this gets too big maybe make it a little bitmask (too big meaning > 8 bytes)
typedef struct {
    bool auto_start; // starting RUNS
    bool vsync;      // vsunc 
} settings;

typedef enum {
    of_Undamaged,
    of_DamagedPreFrame,
    of_DamagedOnFrame,
    of_DamagedPostFrame
} of_state;

typedef enum {
    menu_Main,
    menu_Debug,
    menu_LastRun,
    menu_Settings,
} menu_state;

const char *get_of_state_string(of_state state) {
    assert(state >= 0 && state <= of_DamagedPostFrame);
    const char *states[] = {
        "of_Undamaged",
        "of_DamagedPreFrame",
        "of_DamagedOnFrame",
        "of_DamagedPostFrame"
    };
    return states[state];
}

typedef struct {
    bool should_update_animation_change;
    of_state state;
} of_state_machine;

typedef struct {
    of_state_machine* of_state_machine;
    game_values gameval;
    game_values oldgameval;
    memory_reader reader;
    run* runs;
    nk_context *ctx;
    menu_state menu_state;
    settings settings;
    
    bool is_in_cb;
    cb current_cb;
    
    bool is_in_run;
    run current_run;
} bfbb_stat_tracker;

void update_and_render(bfbb_stat_tracker *);
void run_application();

#endif //BFBB_STAT_TRACKER_H
