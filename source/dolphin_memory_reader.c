
#define is_loading_address         0x3CB7B3
#define bools_address              0x3C2030
#define is_bowling_address         0x3C2047
#define can_cruise_bubble_address  0x3C0F16
#define can_bubble_bowl_address    0x3C0F15
#define frame_oscillator_address   0x3CB854
#define spat_count_address         0x3C205F
#define player_pointer_address     0x3C0F1C
#define level_address              0x28060B
#define update_dt_address          0x3CAAD4
#define bubble_bowl_speed_address  0x3C1F40
#define buttons_address            0x29261E
#define health_address             0x3c1c04
#define can_jump_address           0x3c2020
#define fuse_count_address         0x595b15

#define ReadValue(value_name) read_game_memory(reader, value_name##_address, &value_name, sizeof(value_name))
#define ReadGameValue(value_name) read_game_memory(reader, value_name##_address, &values->value_name, sizeof(values->value_name))
#define ReadGameValueFromPtr(value_name) read_game_memory_from_pointer(reader, value_name##_address, &values->value_name, sizeof(values->value_name))

void read_game_memory_from_pointer(memory_reader *reader, u64 address, void *result, size_t size) {
    // NOTE: check to make sure the pointer is actually in memory
    if (address >= DOLPHIN_BASE_ADDRESS && address < DOLPHIN_BASE_ADDRESS + DOLPHIN_MEM_SIZE) {
        address -= DOLPHIN_BASE_ADDRESS;
        read_game_memory(reader, address, result, size);
    }
}

u32 read_game_memory_get_pointer(memory_reader *reader, u64 address) {
    u32 result = 0;
    read_game_memory_from_pointer(reader, address, &result, sizeof(u32));
    byte_swap_u32(&result);
    return result;
}


// TODO: change "address" to "offset" where applicable and make this platform INDEPENDENT
void get_game_values(memory_reader *reader, game_values *values) {
    ReadGameValue(bools);

    ReadGameValue(can_cruise_bubble);
    ReadGameValue(can_bubble_bowl);
    ReadGameValue(can_jump);
    byte_swap_u32(&values->can_jump);

    ReadGameValue(frame_oscillator);
    byte_swap_u32(&values->frame_oscillator);

    ReadGameValue(spat_count);

    ReadGameValue(health);
    byte_swap_u32(&values->health);

    ReadGameValue(player_pointer);
    byte_swap_u32(&values->player_pointer);

    read_game_memory(reader, level_address, values->level, 4);

    ReadGameValue(update_dt);
    byte_swap_u32((u32 *)&values->update_dt);
    ReadGameValue(bubble_bowl_speed);
    byte_swap_u32((u32 *)&values->bubble_bowl_speed);
    ReadGameValue(buttons);
    byte_swap_u32(&values->buttons);

    u32 character_address = values->player_pointer;
    ReadGameValueFromPtr(character);

    u32 model_instance_spongebob_pointer_address = 0x3c1bf8 + DOLPHIN_BASE_ADDRESS;

    // YEP
    u32 x_anim_state_address = read_game_memory_get_pointer(reader,  read_game_memory_get_pointer(reader, read_game_memory_get_pointer(reader,  read_game_memory_get_pointer(reader, model_instance_spongebob_pointer_address) + 12) + 8) + 4);
    if (x_anim_state_address) {
        u32 anim_id_address = x_anim_state_address + 8;
        ReadGameValueFromPtr(anim_id);
        byte_swap_u32(&values->anim_id);
    }

    // check either game start
    bool game_start_without_autosave;
    bool game_start_with_autosave;
    read_game_memory(reader, 0x541e9c, &game_start_without_autosave, 1);
    read_game_memory(reader, 0x55d6c0, &game_start_with_autosave, 1);
    values->game_start = game_start_with_autosave || game_start_without_autosave;

    ReadGameValue(fuse_count);
}
