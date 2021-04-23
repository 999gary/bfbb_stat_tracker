#include "Leroy.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "nuklear_sdl_gl2.h"


void start_nk_loop(bfbb_stat_tracker* stat_tracker) {
    SDL_Window *win;
    SDL_GLContext glContext;
    int running = 1;
    
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    win = SDL_CreateWindow("BFBB stat tracker",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        window_width, window_height, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_ALLOW_HIGHDPI);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &window_width, &window_height);

    stat_tracker->ctx = nk_sdl_init(win);
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    struct nk_font *droid = nk_font_atlas_add_from_memory(atlas, (char *)LeroyLetteringLightBeta01, sizeof(LeroyLetteringLightBeta01), 15, 0);
    nk_sdl_font_stash_end();
    nk_style_set_font(stat_tracker->ctx, &droid->handle);

    while (running)
    {
        SDL_Event evt;
        nk_input_begin(stat_tracker->ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) goto cleanup;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(stat_tracker->ctx);

        update_and_render(stat_tracker);
        SDL_GetWindowSize(win, &window_width, &window_height);
        glViewport(0, 0, window_width, window_height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(1, 0, 1, 1);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_GL_SwapWindow(win);
    }
    cleanup:
        nk_sdl_shutdown();
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(win);
        SDL_Quit();
}   

void set_vsync(bool on){

}

int main() {run_application();}