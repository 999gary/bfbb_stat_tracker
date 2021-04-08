

typedef struct memory_reader memory_reader;
struct memory_reader {
    bool is_hooked;
    bool should_hook;
    size_t emulated_base_address;
};

void init_memory_reader(memory_reader *reader) {

}

void get_game_values(memory_reader *reader, game_values *values) {

}