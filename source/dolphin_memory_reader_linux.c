
#include <sys/uio.h>
#include <dirent.h>

typedef struct memory_reader memory_reader;
struct memory_reader {
    bool is_hooked;
    bool should_hook;
    size_t emulated_base_address;
    int dolphin;
};

void byte_swap_u16(u16 *n) { *n = __builtin_bswap16(*n); }
void byte_swap_u32(u32 *n) { *n = __builtin_bswap32(*n); }
void byte_swap_u64(u64 *n) { *n = __builtin_bswap64(*n); }

#define MAX_PATH_LENGTH 4096

// NOTE(jelly): this code makes me very sad
void init_memory_reader(memory_reader *reader) {
    DIR *dir = opendir("/proc/");
    // TODO(jelly): check dir for null. /proc should always be on a linux machine but just in case we can't open it or something idk

    // TODO(jelly): use strncat instead or something better
    //              maybe write a little string builder interface in C
    bool is_hooked = false;
    for (struct dirent *dir_entry = NULL; (dir_entry = readdir(dir)) && !is_hooked; ) {
        char *d_name = dir_entry->d_name;
        int pid = atoi(d_name);
        if (!pid) continue;
        static char pid_d_path[MAX_PATH_LENGTH];
        strncpy(pid_d_path, "/proc/", MAX_PATH_LENGTH);
        strcat(pid_d_path, d_name);

        static char comm_d_path[MAX_PATH_LENGTH];
        strncpy(comm_d_path, pid_d_path, MAX_PATH_LENGTH);
        strcat(comm_d_path, "/comm");

        FILE *comm = fopen(comm_d_path, "r");
        if (comm) {
            char line[256];
            if (fgets(line, 256, comm)) {
                if (!strcmp(line, "dolphin-emu\n") ||
                    !strcmp(line, "dolphin-emu-qt2\n") ||
                    !strcmp(line, "dolphin-emu-wx\n")) {
                    static char maps_path[MAX_PATH_LENGTH];
                    strncpy(maps_path, pid_d_path, MAX_PATH_LENGTH);
                    strcat(maps_path, "/maps");
                    FILE *maps = fopen(maps_path, "r");
                    if (maps) {
                        while (fgets(line, 256, maps)) {
                            char tmp[256];
                            strncpy(tmp, line, 256);
                            char *tok = strtok(tmp, " ");
                            bool is_line = false;
                            while (tok) {
                                char *thing1 = "/dev/shm/dolphinmem";
                                char *thing2 = "/dev/shm/dolphin-emu";
                                if (!strncmp(tok, thing1, strlen(thing1)) || !strncmp(tok, thing2, strlen(thing2))) {
                                    is_line = true;
                                    break;
                                }
                                tok = strtok(NULL, " ");
                            }
                            if (is_line) {
                                strncpy(tmp, line, 256);
                                char *first_address_str = strtok(tmp, " -");
                                char *second_address_str = strtok(NULL, " -");
                                unsigned long first_address = strtoul(first_address_str, NULL, 16);
                                unsigned long second_address = strtoul(second_address_str, NULL, 16);
                                assert(first_address && second_address);
                                unsigned long size = second_address - first_address;
                                if (size == DOLPHIN_MEM_SIZE) {
                                    printf("dolphin pid = %d\n", pid);
                                    reader->dolphin = pid;
                                    reader->emulated_base_address = first_address;
                                    reader->is_hooked = true;
                                    is_hooked = true;
                                }
                                break;
                            }
                        }

                        fclose(maps);
                    }
                }
            }
            fclose(comm);
        }
    }
    closedir(dir);
}

// NOTE(jelly): on linux, it seems like we could sort of "queue" up different values to read
//              and then call process_vm_readv ONCE every frame
//              maybe we could write a similarish interface on windows that maybe reads entire
//              chunk of memory with ReadProcessMemory and sets all the values
//              it could be worth looking in to for performance.
void read_game_memory(memory_reader *reader, u64 address, void *result, size_t size) {
    struct iovec dest;
    dest.iov_base = result;
    dest.iov_len = size;

    struct iovec src;
    src.iov_base = reader->emulated_base_address + address;
    src.iov_len = size;

    ssize_t bytes_read = process_vm_readv(reader->dolphin, &dest, 1, &src, 1, 0);
    reader->is_hooked = bytes_read == size;
}
