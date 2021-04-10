
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

s32 window_width = 800;
s32 window_height = 700;

float line_thickness = 4.0f;
float graph_dot_thickness = 10;

#if defined(_WIN32)
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_D3D9_IMPLEMENTATION
#include "nuklear.h"
#else
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_GL2_IMPLEMENTATION
#include "nuklear.h"
#endif
typedef struct nk_context nk_context;
#include "cJSON.c"
#if defined(_WIN32)
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
#else
typedef union {
    struct {
        s32 is_jumping;
        s32 is_d_jumping;
        s32 is_bubble_spinning;
        s32 is_bubble_bouncing;
        s32 is_bubble_bashing;
        s32 is_bubble_bowling;
        s32 was_d_jumping;
        s32 is_coptering;
    };
    s32 all[8];
} player_bools;
#endif


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
    bool game_start; // this will check for w/ and w/o autosave because we don't care which one
    u8 fuse_count; // in da brain
} game_values;

int of_counter = 0;
int of_frameon = 0;
int of_framepress = 0;
bool should_count = false;

// NOTE(jelly): this is a bit of a mess
#if  defined(DOLPHIN)
#include "dolphin_memory_reader.h"
#if  defined(_WIN32)
#include "dolphin_memory_reader_win32.c"
#elif defined(__linux__)
#include "dolphin_memory_reader_linux.c"
#endif
#include "dolphin_memory_reader.c"
#elif defined(XBOX)
#include "xbox_memory_reader.c"
#else
#error PLEASE DEFINE A PLATFORM!!!!
#endif

#include "bfbb_stat_tracker.h"

#if defined(_WIN32)
//#include "win32_gdi_renderer.c"
#include "win32_d3d9_renderer.c"
#elif defined(__linux__)
#include <float.h>
#include "linux_sdl_renderer.c"
#else
#error UNKNOWN RENDER PLATFORM!!!!
#endif

#include "style.c"
#include "level_names.h"

void load_runs(bfbb_stat_tracker* tracker) {
    FILE* file = fopen("runs.json", "r");
    if (file == NULL) 
        return;
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    char *string = malloc(size + 1);
    fseek(file, 0, SEEK_SET);
    fread(string, 1, size, file);
    string[size] = '\0';
    fclose(file);

    cJSON* main_json = cJSON_Parse(string);
    if (main_json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return;
    }

    cJSON* run_count = cJSON_GetObjectItemCaseSensitive(main_json, "run_count");
    if(!cJSON_IsNumber(run_count) || run_count->valuedouble == 0) 
        return;

    cJSON* run_array = cJSON_GetObjectItemCaseSensitive(main_json, "run_array");
    if(!cJSON_IsArray(run_array))
        return;


    int runs_count = run_count->valueint;
    cJSON* run_element;

    cJSON_ArrayForEach(run_element, run_array) {
        run run_temp = {0};
        cJSON* player_bools = cJSON_GetObjectItemCaseSensitive(run_element, "player_bools");
        run_temp.bool_counts.is_jumping = cJSON_GetObjectItemCaseSensitive(player_bools, "is_jumping")->valueint;
        run_temp.bool_counts.is_d_jumping = cJSON_GetObjectItemCaseSensitive(player_bools, "is_d_jumping")->valueint;
        run_temp.bool_counts.is_bubble_spinning = cJSON_GetObjectItemCaseSensitive(player_bools, "is_bubble_spinning")->valueint;
        run_temp.bool_counts.is_bubble_bouncing = cJSON_GetObjectItemCaseSensitive(player_bools, "is_bubble_bouncing")->valueint;
        run_temp.bool_counts.is_bubble_bashing = cJSON_GetObjectItemCaseSensitive(player_bools, "is_bubble_bashing")->valueint;
        run_temp.bool_counts.is_bubble_bowling = cJSON_GetObjectItemCaseSensitive(player_bools, "is_bubble_bowling")->valueint;
        run_temp.bool_counts.was_d_jumping = cJSON_GetObjectItemCaseSensitive(player_bools, "was_d_jumping")->valueint;
        run_temp.bool_counts.is_coptering = cJSON_GetObjectItemCaseSensitive(player_bools, "is_coptering")->valueint;
        run_temp.cb_average_speed = cJSON_GetObjectItemCaseSensitive(run_element, "cb_average_speed")->valuedouble;
        run_temp.frame_count = cJSON_GetObjectItemCaseSensitive(run_element, "frame_count")->valueint;
        
        run_temp.cruise_boosts;
        cJSON* cbs_array = cJSON_GetObjectItemCaseSensitive(run_element, "cbs_array");
        cJSON* cb_element;
        cJSON_ArrayForEach(cb_element, cbs_array) {
            cb cruise_boost = {0};
            cruise_boost.speed = cJSON_GetObjectItemCaseSensitive(cb_element, "cb_speed")->valuedouble;
            cruise_boost.startframe = cJSON_GetObjectItemCaseSensitive(cb_element, "cb_start_frame")->valueint;
            cruise_boost.endframe = cJSON_GetObjectItemCaseSensitive(cb_element, "cb_end_frame")->valueint;
            sb_push(run_temp.cruise_boosts, cruise_boost);
        }
        sb_push(tracker->runs, run_temp);
    }

    cJSON_Delete(main_json);
    free(string);
}

cJSON* create_run_json(bfbb_stat_tracker* stat_tracker, int run) {
    cJSON* main = cJSON_CreateObject();
    cJSON* player_bools = cJSON_CreateObject();
    
    //TODO (Will): Error check these?
    
    if(cJSON_AddNumberToObject(player_bools, "is_jumping", (double)stat_tracker->runs[run].bool_counts.is_jumping) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "is_d_jumping", (double)stat_tracker->runs[run].bool_counts.is_d_jumping) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "is_bubble_spinning", (double)stat_tracker->runs[run].bool_counts.is_bubble_spinning) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "is_bubble_bouncing", (double)stat_tracker->runs[run].bool_counts.is_bubble_bouncing) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "is_bubble_bashing", (double)stat_tracker->runs[run].bool_counts.is_bubble_bashing) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "is_bubble_bowling", (double)stat_tracker->runs[run].bool_counts.is_bubble_bowling) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "was_d_jumping", (double)stat_tracker->runs[run].bool_counts.was_d_jumping) == NULL) {}
    if(cJSON_AddNumberToObject(player_bools, "is_coptering", (double)stat_tracker->runs[run].bool_counts.is_coptering) == NULL) {}
    if(cJSON_AddItemToObject(main, "player_bools", player_bools) == true) {}
    if(cJSON_AddNumberToObject(main, "frame_count", (double)stat_tracker->runs[run].frame_count)) {}
    if(cJSON_AddNumberToObject(main, "cb_average_speed", (double)stat_tracker->runs[run].cb_average_speed)) {}
    
    //TODO (Will): Check if we are in a cb maybe???????
    int cb_count = sb_count(stat_tracker->runs[run].cruise_boosts);
    
    if(cJSON_AddNumberToObject(main, "cb_count", cb_count)) {}
    cJSON* cbs_array = cJSON_CreateArray();
    if(cJSON_AddItemToObject(main, "cbs_array", cbs_array)) {}
    for(int i = 0; i < cb_count; i++) {
        cJSON* cb_object = cJSON_CreateObject(); 
        if(cJSON_AddItemToArray(cbs_array, cb_object)) {}
        cJSON* cb_start_frame = cJSON_CreateNumber((double)stat_tracker->runs[run].cruise_boosts[i].startframe);
        if(cJSON_AddItemToObject(cb_object, "cb_start_frame", cb_start_frame)) {}
        cJSON* cb_end_frame = cJSON_CreateNumber((double)stat_tracker->runs[run].cruise_boosts[i].endframe);
        if(cJSON_AddItemToObject(cb_object, "cb_end_frame", cb_end_frame)) {}
        cJSON* cb_speed = cJSON_CreateNumber((double)stat_tracker->runs[run].cruise_boosts[i].speed);
        if(cJSON_AddItemToObject(cb_object, "cb_speed", cb_speed)) {}
    }
    return main;
}


bool create_runs_json_file(bfbb_stat_tracker* stat_tracker) {
    FILE* file = fopen("runs.json", "w");
    cJSON* main = cJSON_CreateObject(); 
    int runs_count = sb_count(stat_tracker->runs);
    cJSON* run_count = cJSON_CreateNumber((double)runs_count);
    if(cJSON_AddItemToObject(main, "run_count", run_count)) {}
    cJSON* run_array = cJSON_CreateArray();
    if(cJSON_AddItemToObject(main, "run_array", run_array)) {}
    for(int i = 0; i < runs_count; i++) {
        cJSON* run = create_run_json(stat_tracker, i);
        if(cJSON_AddItemToArray(run_array, run)) {};
    }
    
    char* string = cJSON_Print(main);
    
    fwrite(string, 1, strlen(string), file);
    
    fclose(file);
}

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
    create_runs_json_file(tracker);
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
    set_vsync(stat_tracker->settings.vsync); // where is a good place to put this ???
    
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
        
        run *current_run = &stat_tracker->current_run;
        cb *current_cb = &stat_tracker->current_cb;
        
        game_values *gameval = &stat_tracker->gameval;
        
        if (stat_tracker->oldgameval.frame_oscillator != stat_tracker->gameval.frame_oscillator)
        {
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
    
    nk_clear(ctx);
    
    if(nk_begin(ctx, "Yep", nk_rect(0, 0, window_width, window_height), NK_WINDOW_NO_SCROLLBAR))
    {
        stat_tracker->reader.should_hook = true; // TODO: get rid of this TBH
        switch(stat_tracker->menu_state) {
            case(menu_Main): {
                bool dolphin_is_hooked  = stat_tracker->reader.is_hooked;
                
                
                
                
                if (dolphin_is_hooked) {
                    if(!stat_tracker->settings.auto_start) {
                        nk_layout_row_static(ctx, 30, window_width, 1);
                        if (stat_tracker->is_in_run) {
                            if (nk_button_label(ctx, "End Run")) {
                                end_run(stat_tracker);
                            }
                        } else {
                            if (nk_button_label(ctx, "Start Run")) {
                                start_run(stat_tracker);
                            }
                        }
                    }
                }
                if (sb_count(stat_tracker->runs) && !stat_tracker->is_in_run) {
                    nk_layout_row_static(ctx, 30, window_width, 1);
                    if (nk_button_label(ctx, "Run History")) {
                        stat_tracker->menu_state = menu_LastRun;
                    }
                }
                nk_layout_row_static(ctx, 30, window_width/2, 2);
                if (nk_button_label(ctx, "Debug")) {
                    stat_tracker->menu_state = menu_Debug;
                }
                if (nk_button_label(ctx, "Settings")) {
                    stat_tracker->menu_state = menu_Settings;
                }
                
                if (sb_count(stat_tracker->runs) && !stat_tracker->is_in_run) {
                    run last_run = sb_last(stat_tracker->runs);
                    nk_layout_row_static(ctx, 30, window_width/2, 1);
                    nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "End Frame: %d", last_run.frame_count);
                    nk_layout_row_static(ctx, 30, window_width/2, 1);
                    nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(last_run.cruise_boosts) + stat_tracker->is_in_cb);
                    
                    nk_layout_row_static(ctx, 30, window_width/2, 2);
                    nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %g", last_run.cb_average_speed);
                    
                    
                    
                    int index = -1;
                    int cbcount = sb_count(stat_tracker->current_run.cruise_boosts);
                    float min = FLT_MAX;
                    float max = 0;
                    for(int i = 0; i<cbcount; i++) {
                        float speed = stat_tracker->current_run.cruise_boosts[i].speed;
                        if(speed < min) {
                            min = speed;
                        } else if (speed > max) {
                            max = speed;
                        }
                    }
                    
                    
                    if (index != -1)
                        nk_tooltipf(ctx, "Speed: %g", stat_tracker->current_run.cruise_boosts[index].speed);
                    player_bools bool_counts = last_run.bool_counts;
                    
                    nk_layout_row_dynamic(ctx, 30*6, 1);
                    if(nk_group_begin(ctx, "Useless Stats", NK_PANEL_MENU)) {
                        nk_layout_row_static(ctx, 30, window_width/2, 2);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Bashes: %u", bool_counts.is_bubble_bashing);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Bounces: %u", bool_counts.is_bubble_bouncing);
                        nk_layout_row_static(ctx, 30, window_width/2, 2);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Bowls: %u", bool_counts.is_bubble_bowling);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Copters: %u", bool_counts.is_coptering);
                        nk_layout_row_static(ctx, 30, window_width/2, 2);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Double Jumps: %u", bool_counts.is_d_jumping);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Jumps: %u", bool_counts.is_jumping);
                        nk_layout_row_static(ctx, 30, window_width/2, 2);
                        nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Spins: %u", bool_counts.is_bubble_spinning);
                        nk_group_end(ctx);
                    }
                    
                    nk_layout_row_static(ctx, 30, window_width, 1);
                    if (nk_button_label(ctx, "Graphs")) {
                        stat_tracker->menu_state = menu_Debug;
                    }
                    
                    nk_layout_row_dynamic(ctx, 150, 1);
                    if(nk_chart_begin(ctx, NK_CHART_LINES, sb_count(stat_tracker->current_run.cruise_boosts), min, max)) {
                        for(int i = 0; i<cbcount; i++) {
                            nk_flags res = nk_chart_push(ctx, stat_tracker->current_run.cruise_boosts[i].speed);
                            if (res & NK_CHART_HOVERING)
                                index = i;
                        }
                        
                        nk_chart_end(ctx);
                    }
                    
                }
                //TODO: Make this colored
                nk_layout_row_static(ctx, 30, window_width, 1);
                nk_label_colored(ctx, dolphin_is_hooked ? "Dolphin is hooked":"Dolphin is NOT hooked", NK_TEXT_ALIGN_LEFT, dolphin_is_hooked ? nk_rgb(0, 255, 0) : nk_rgb(255, 0, 0));
            } break;
            case(menu_LastRun): {
                run last_run = sb_last(stat_tracker->runs);
                nk_layout_row_static(ctx, 30, window_width, 1);
                if (nk_button_label(ctx, "Back")) {
                    stat_tracker->menu_state = menu_Main;
                }
                nk_layout_row_static(ctx, 30, window_width/2, 1);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "End Frame: %d", last_run.frame_count);
                nk_layout_row_static(ctx, 30, window_width/2, 1);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Count: %d", sb_count(last_run.cruise_boosts) + stat_tracker->is_in_cb);
                
                nk_layout_row_static(ctx, 30, window_width/2, 2);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed Average: %g", last_run.cb_average_speed);
                
                nk_layout_row_dynamic(ctx, 150, 1);
                
                int index = -1;
                int cbcount = sb_count(stat_tracker->current_run.cruise_boosts);
                float min = FLT_MAX;
                float max = 0;
                for(int i = 0; i<cbcount; i++) {
                    float speed = stat_tracker->current_run.cruise_boosts[i].speed;
                    if(speed < min) {
                        min = speed;
                    } else if (speed > max) {
                        max = speed;
                    }
                }
                if(nk_chart_begin(ctx, NK_CHART_LINES, sb_count(stat_tracker->current_run.cruise_boosts), min, max)) {
                    for(int i = 0; i<cbcount; i++) {
                        nk_flags res = nk_chart_push(ctx, stat_tracker->current_run.cruise_boosts[i].speed);
                        if (res & NK_CHART_HOVERING)
                            index = i;
                    }
                    
                    nk_chart_end(ctx);
                }
                
                if (index != -1)
                    nk_tooltipf(ctx, "Speed: %g", stat_tracker->current_run.cruise_boosts[index].speed);
                player_bools bool_counts = last_run.bool_counts;
                
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
            } break;
            case(menu_Debug): {
                if (stat_tracker->is_in_run) {
                    float speeds[] = {
                        1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 4.5, 2.3, 4.5, 7.9, 16.8
                    };
                    
                    nk_layout_row_static(ctx, 30, window_width, 1);
                    if (nk_button_label(ctx, "Add Test Cruise Boosts")) {
                        cb tmp = {0};
                        int count = 20;
                        float sum = 0.0f;
                        for (s32 i = 0; i < count; ++i) {
                            tmp.startframe = tmp.endframe;
                            tmp.endframe = tmp.startframe + rand()%1200 + 120;
                            tmp.speed = speeds[i%ArrayCount(speeds)];
                            sum = tmp.speed;
                            sb_push(stat_tracker->current_run.cruise_boosts, tmp);
                        }
                        float avg = sum / (float)count;
                        stat_tracker->current_run.cb_average_speed = avg;
                    }
                }
                
                
                nk_layout_row_static(ctx, 30, window_width, 1);
                if (nk_button_label(ctx, "Back")) {
                    stat_tracker->menu_state = menu_Main;
                }
                if (stat_tracker->gameval.level[0]) nk_label(ctx, get_level_name(stat_tracker->gameval.level), NK_TEXT_ALIGN_LEFT);
                nk_layout_row_static(ctx, 30, window_width/3, 1);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "Buttons: %d", stat_tracker->gameval.buttons);
                nk_layout_row_static(ctx, 30, window_width/3, 1);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "CB Speed: %g", stat_tracker->gameval.bubble_bowl_speed);
                nk_layout_row_static(ctx, 30, window_width/3, 1);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "OF State: %d", stat_tracker->of_state_machine->state);
                nk_layout_row_static(ctx, 30, window_width/3, 1);
                nk_label_printf(ctx, NK_TEXT_ALIGN_LEFT, "OF Missed: %d", of_frameon-of_framepress);
            } break;
            case(menu_Settings): {
                nk_layout_row_static(ctx, 30, window_width, 1);
                if (nk_button_label(ctx, "Back")) {
                    stat_tracker->menu_state = menu_Main;
                }
                
                nk_bool tmp = stat_tracker->settings.auto_start;
                nk_layout_row_static(ctx, 30, window_width, 1);
                nk_checkbox_label(ctx, "Auto Start Run", &tmp);
                stat_tracker->settings.auto_start = tmp;
                
                tmp = stat_tracker->settings.vsync;
                nk_layout_row_static(ctx, 30, window_width, 1);
                nk_checkbox_label(ctx, "Vsync", &tmp);
                stat_tracker->settings.vsync = tmp;
            } break;
        }
        
        /*
                nk_layout_row_static(ctx, 30, window_width/2, 2);

                if(stat_tracker->is_in_run) {
                    run currentrun = stat_tracker->current_run;
                    nk_layout_row_static(ctx, 30, window_width/3, 1);
                    nk_label(ctx, "Current Run:", NK_TEXT_ALIGN_LEFT);

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
                */
        set_style(ctx, THEME_BOB);
    }
    nk_end(ctx);
}



void run_application(void) {
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
    stat_tracker.menu_state = menu_Main;
    
    // vsync should be ON by default
    stat_tracker.settings.vsync = 1;
    
    
    load_runs(&stat_tracker);
    
    start_nk_loop(&stat_tracker);
}
