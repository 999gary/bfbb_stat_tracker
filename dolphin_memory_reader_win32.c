
// TODO: loop through different exe names .. Dolphin.exe, DolphinWx.exe, DolphinQt2.exe

#define DOLPHIN_PROCESS_NAME "Dolphin.exe"
#define DOLPHIN_BASE_ADDRESS 0x80000000
#define DOLPHIN_MEM_SIZE     0x2000000

void byte_swap_u16(u16 *n) { *n = _byteswap_ushort(*n); }
void byte_swap_u32(u32 *n) { *n = _byteswap_ulong(*n); }
void byte_swap_u64(u64 *n) { *n = _byteswap_uint64(*n); }

u64 get_emulated_base_address(HANDLE dolphin) {
    MEMORY_BASIC_INFORMATION info;
    u8 *memory = NULL;
    while (VirtualQueryEx(dolphin, memory, &info, sizeof(info)) == sizeof(info)) {
        if (info.RegionSize == DOLPHIN_MEM_SIZE && info.Type == MEM_MAPPED) {
            PSAPI_WORKING_SET_EX_INFORMATION wsInfo;
            wsInfo.VirtualAddress = info.BaseAddress;
            if (QueryWorkingSetEx(dolphin, &wsInfo, sizeof(PSAPI_WORKING_SET_EX_INFORMATION))) {
                if (wsInfo.VirtualAttributes.Valid) {
                    return (u64)info.BaseAddress;
                }
            }
        }
        
        memory += info.RegionSize;
    }
    
    return 0;
}

typedef struct memory_reader memory_reader;
struct memory_reader {
    bool is_hooked;
    bool should_hook;
    HANDLE dolphin;
    size_t emulated_base_address;
};

void init_memory_reader(memory_reader *reader) {
    if (!reader->should_hook || reader->is_hooked)
        return;
    char *p_name = DOLPHIN_PROCESS_NAME;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 structprocsnapshot = { 0 };
    
    structprocsnapshot.dwSize = sizeof(PROCESSENTRY32);
    
    if (snapshot == INVALID_HANDLE_VALUE) return;
    if (Process32First(snapshot, &structprocsnapshot) == FALSE) return;
    
    while (Process32Next(snapshot, &structprocsnapshot)) {
        if (!strcmp(structprocsnapshot.szExeFile, p_name)) {
            DWORD pid = structprocsnapshot.th32ProcessID;
            HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, pid);
            u64 emulated_base_address = get_emulated_base_address(process_handle);
            if (emulated_base_address) {
                reader->dolphin = process_handle;
                reader->emulated_base_address = emulated_base_address;
                reader->is_hooked = true;
                CloseHandle(snapshot);
                return;
            }
        }
    }
    CloseHandle(snapshot);
    return;
}

void read_game_memory(memory_reader *reader, u64 address, void *result, size_t size) {
    // should we check if we're hooked every time??? for now, it seems like we don't have to.
    // calling ReadProcessMemory simply fails and that's fine i guess
    
    size_t bytes_read;
    BOOL rc = ReadProcessMemory(reader->dolphin, (LPCVOID)(reader->emulated_base_address + address), result, size, &bytes_read);
    
    // We've probably lost dolphin now
    reader->is_hooked = rc && (bytes_read == size);
}

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