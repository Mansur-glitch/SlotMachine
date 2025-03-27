// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "configuration.hpp"
#include "game.hpp"
#include "graphics_system.hpp"
#include "scene.hpp"
#include "symbol.hpp"
#include "texture.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_main.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <ios>
#include <iostream>
#include <limits>
#include <string>
#include <thread>


class App
{
public:
  App()
  {
    m_tc.load_predefined(m_gs);

    m_gs.set_background_color(g_window_color);

    m_game.reset(new Game(m_tc));

    // Trigger next reels state using console input
    if constexpr (g_testing_enabled) {
      static const char* help_message = "Enter %d numbers from 0 to %d to move reels to corresponding symbols. Enter 'q' to exit.";

      m_input_thread.reset(new std::thread([this]() {
        Game::SymbolRow row;
        uint32_t i = 0;

        while (!m_quit_flag) {
          char c{'\0'};
          std::cin.get(c);
          if (c == 'q') { // enter symbol to exit program 
            m_quit_flag = true;
            return;
          }
          std::cin.putback(c);

          int num;
          if (! (std::cin >> num)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            SDL_Log(help_message, g_nreels, g_nsymbols - 1);
          } else {
            row[i] = static_cast<Symbol>(num % g_nsymbols);
            i = (i + 1) % row.size();

            if (i == 0) { // full row
              m_game->set_symbol_row(row);
            }
          }
        }
      }));
      SDL_Log(help_message, g_nreels, g_nsymbols - 1);
    }

    m_update_time = std::chrono::steady_clock::now();
  }

  ~App()
  {
    if constexpr (g_testing_enabled) {
      m_input_thread->join();
    }
  }

  void run()
  {
    SDL_Event e;
    SDL_zero(e);

    while (!m_quit_flag) {
      while (SDL_PollEvent(&e)) {
        switch (e.type) {
          case SDL_EVENT_QUIT:
            m_quit_flag = true;
            if constexpr (g_testing_enabled) {
              // std::cin is blocked. Still waiting user enter something
              SDL_Log("Enter any character to finish program");
            }
            break;
          case SDL_EVENT_WINDOW_RESIZED:
            m_wnd_width = e.window.data1;
            m_wnd_height = e.window.data2;
            break;
          case SDL_EVENT_MOUSE_MOTION:
          case SDL_EVENT_MOUSE_BUTTON_UP: {
            m_game->process_input(e);
            break;
          }
          default:
            break;
        }
      }
      TimePoint frame_begin = std::chrono::steady_clock::now();
      FloatSeconds dt = frame_begin - m_update_time;
      m_update_time = frame_begin;

      m_game->update(dt.count());
      m_gs.draw(m_game->get_scene().build(m_wnd_width, m_wnd_height));

      TimePoint frame_end = std::chrono::steady_clock::now();
      FloatSeconds frame_time;
      if (frame_time < g_standard_frame_time) {
        std::this_thread::sleep_for(g_standard_frame_time - frame_time);
      }
    }
  }

private:
  bool m_quit_flag {false};
  GraphicsSystem m_gs{ g_wnd_title, g_init_wnd_width, g_init_wnd_height };
  TextureCollection m_tc{ "image_resources", 32 };
  uint16_t m_wnd_width{ g_init_wnd_width };
  uint16_t m_wnd_height{ g_init_wnd_height };
  TimePoint m_update_time;
  std::unique_ptr<Game> m_game{};
  std::unique_ptr<std::thread> m_input_thread{};
};


int main(int argc, char* argv[])
{
  try {
    App app;
    app.run();
  } catch (std::exception& ex) {
    SDL_Log("%s", ex.what());
  } catch (...) {
    SDL_Log("Unrecognized error.");
  }
  return 0;
}
