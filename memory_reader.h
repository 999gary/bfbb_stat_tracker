#ifndef MEMORY_READER_H
#define MEMORY_READER_H

#define BUTTON_L (64)
#define BUTTON_X (1024)

typedef struct {
    bool is_loading;
    bool is_bowling;
    bool can_cruise_bubble;
    bool can_bubble_bowl;
    u8 spat_count;
    u32 player_pointer;
    u32 character;
    char level[8];
    float update_dt;
    float bubble_bowl_speed;
    u32 buttons;
} game_values;

#endif //MEMORY_READER_H
