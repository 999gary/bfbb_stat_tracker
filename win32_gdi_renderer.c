
#include "soonge_font.h"

#include "nuklear_gdip.h"

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    bfbb_stat_tracker *idk = (bfbb_stat_tracker *)GetWindowLongPtr(wnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_PAINT: {
            update_and_render(idk);
            nk_gdip_render(NK_ANTI_ALIASING_ON, nk_rgb(30,30,30));
        } break;
        case WM_SIZE: {
            window_width  = LOWORD(lparam);
            window_height = HIWORD(lparam);
        } break;
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    if (nk_gdip_handle_event(wnd, msg, wparam, lparam))
        return 0;
    return DefWindowProcW(wnd, msg, wparam, lparam);
}


void start_nk_loop(bfbb_stat_tracker* idk) {
    GdipFont* font;
    
    WNDCLASSW wc;
    RECT rect = { 0, 0, window_width, window_height };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;    
    HWND wnd;
    int running = 1;
    int needs_refresh = 1;
    
    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"BFBB Stat Tracker Window Class :))))";
    RegisterClassW(&wc);
    
    AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    
    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"BFBB Stat Tracker",
                          style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                          rect.right - rect.left, rect.bottom - rect.top,
                          NULL, NULL, wc.hInstance, NULL);
    
    SetWindowLongPtr(wnd, GWLP_USERDATA, idk);
    
    /* GUI */
    idk->ctx = nk_gdip_init(wnd, window_width, window_height);
    
    struct nk_context *ctx = idk->ctx;
    
    font = nk_gdipfont_create_from_memory(soonge, sizeof(soonge), 17);
    nk_gdip_set_font(font);
    while (running)
    {
        MSG msg;
        nk_input_begin(ctx);
        if (needs_refresh == 0) {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0)
                running = 0;
            else {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            needs_refresh = 1;
        } else needs_refresh = 0;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        needs_refresh = 1;
        nk_input_end(ctx);
        
        update_and_render(idk);
        
        nk_gdip_render(NK_ANTI_ALIASING_ON, nk_rgb(255,0,255));
        
        Sleep(3);
    }
    nk_gdipfont_del(font);
    nk_gdip_shutdown();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}