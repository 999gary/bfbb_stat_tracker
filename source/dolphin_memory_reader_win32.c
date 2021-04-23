
// TODO: loop through different exe names .. Dolphin.exe, DolphinWx.exe, DolphinQt2.exe

#define DOLPHIN_PROCESS_NAME "Dolphin.exe"


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

    reader->is_hooked = rc && (bytes_read == size);
}
