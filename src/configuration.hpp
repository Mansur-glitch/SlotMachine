#ifndef SLOT_MACHINE_CONFIGURATION
#define SLOT_MACHINE_CONFIGURATION
// Instead of loadable configuration file

#include "SDL3/SDL_pixels.h"
#include <chrono>
#include <cstdint>

using TimePoint = std::chrono::steady_clock::time_point;
using FloatSeconds = std::chrono::duration<float>;

// Enable to move reels to specified symbols using console input
static constexpr bool g_testing_enabled = false;

static constexpr const char* g_wnd_title = "Slot machine";
static constexpr uint16_t g_init_wnd_width = 800;
static constexpr uint16_t g_init_wnd_height = 600;
static constexpr uint16_t g_fps_cap = 60;
static constexpr FloatSeconds g_standard_frame_time =
  FloatSeconds(1.f / g_fps_cap);

constexpr uint32_t g_nreels = 5;
constexpr uint32_t g_nlines = 3;

constexpr float g_reel_max_speed = 5.f;  // displays per second
constexpr float g_reel_min_speed = 0.5f; // displays per second
constexpr FloatSeconds g_min_speed_up_time{ 3.f };
constexpr FloatSeconds g_max_speed_up_time{ 6.f };
constexpr FloatSeconds g_min_spin_time = g_min_speed_up_time;
constexpr FloatSeconds g_max_spin_time = g_min_spin_time + FloatSeconds{ 3.f };
constexpr FloatSeconds g_min_stop_time{ 3.f };
constexpr FloatSeconds g_max_stop_time{ 6.f };
constexpr FloatSeconds g_result_show_time{ 2.f };
constexpr FloatSeconds g_auto_spin_delay{ 0.5f };

constexpr SDL_Color g_window_color{ 219, 218, 213, 255 };
constexpr SDL_Color g_main_panel_color{ 223, 148, 18, 255 };
constexpr SDL_Color g_control_panel_color{ 209, 135, 9, 255 };
constexpr SDL_Color g_shadow_color{ 15, 15, 15, 255 };
constexpr SDL_Color g_score_card_color{ 215, 213, 179, 255 };
constexpr SDL_Color g_symbol_card_color{ 215, 179, 194, 255 };
constexpr SDL_Color g_start_en_color{ 38, 156, 21, 255 };
constexpr SDL_Color g_start_hv_color{ 55, 216, 62, 255 };
constexpr SDL_Color g_disabled_color{ 30, 30, 30, 255 };
constexpr SDL_Color g_stop_en_color{ 192, 40, 24, 255 };
constexpr SDL_Color g_stop_hv_color{ 223, 28, 7, 255 };
constexpr SDL_Color g_combo_highlight{ 81, 255, 109, 255 };
#endif
