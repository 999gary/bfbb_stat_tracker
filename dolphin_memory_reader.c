
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#define DOLPHIN_PROCESS_NAME "Dolphin.exe"

u16 byte_swap_u16(u16 n) { return _byteswap_ushort(n); }
u32 byte_swap_u32(u32 n) { return _byteswap_ulong(n); }
u64 byte_swap_u64(u64 n) { return _byteswap_uint64(n); }

DWORD get_process_id(char *p_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 structprocsnapshot = { 0 };
    
    structprocsnapshot.dwSize = sizeof(PROCESSENTRY32);
    
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    if (Process32First(snapshot, &structprocsnapshot) == FALSE) return 0;
    
    while (Process32Next(snapshot, &structprocsnapshot)) {
        if (!strcmp(structprocsnapshot.szExeFile, p_name)) {
            CloseHandle(snapshot);
            return structprocsnapshot.th32ProcessID;
        }
    }
    CloseHandle(snapshot);
    return 0;
}

HANDLE get_dolphin_process_handle(void) {
    HANDLE result = 0;
    DWORD id = get_process_id(DOLPHIN_PROCESS_NAME);
    if (id) {
        result = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);
    }
    return result;
}

u64 get_emulated_base_address(HANDLE dolphin) {
    MEMORY_BASIC_INFORMATION info;
    u8 *memory = NULL;
    while (VirtualQueryEx(dolphin, memory, &info, sizeof(info)) == sizeof(info)) {
        if (info.RegionSize == 0x2000000 && info.Type == MEM_MAPPED) {
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
    HANDLE dolphin;
    size_t emulated_base_address;
};

void init_memory_reader(memory_reader *reader) {
    reader->dolphin = get_dolphin_process_handle();
    reader->emulated_base_address = get_emulated_base_address(reader->dolphin);
    if (reader->dolphin && reader->emulated_base_address) {
        reader->is_hooked = true;
    }
}

void read_game_memory(memory_reader *reader, u64 address, void *result, size_t size) {
    size_t bytes_read;
    BOOL rc = ReadProcessMemory(reader->dolphin, (LPCVOID)(reader->emulated_base_address + address), result, size, &bytes_read);
    assert(rc);
    assert(size == bytes_read);
}

#if 0 
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
#endif

#define is_loading_address         0x3CB7B3
#define is_bowling_address         0x3C0F1C
#define can_cruise_bubble_address  0x3C0F16
#define can_bubble_bowl_address    0x3C0F15
#define spat_count_address         0x3C205F
#define player_pointer_address     0x3C0F1C
#define level_address              0x28060B
#define update_dt_address          0x3CAAD4
#define bubble_bowl_speed_address  0x3C1F40
#define buttons_address            0x29261E

#define ReadGameValue(value_name) read_game_memory(reader, value_name##_address, &values->value_name, sizeof(values->value_name))  

void get_game_values(memory_reader *reader, game_values *values) {
    ReadGameValue(is_bowling);
    ReadGameValue(can_cruise_bubble);
    ReadGameValue(can_bubble_bowl);
    ReadGameValue(spat_count);
    ReadGameValue(player_pointer);
    read_game_memory(reader, level_address, values->level, 4);
    ReadGameValue(update_dt);
    ReadGameValue(bubble_bowl_speed);
    ReadGameValue(buttons);
    
    u32 character_address = byte_swap_u32(values->player_pointer) - 0x80000000;
    read_game_memory(reader, character_address, &values->character, sizeof(values->character));
}