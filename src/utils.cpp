// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "utils.hpp"

#include <SDL3/SDL.h>
#include <cstdarg>

thread_local std::array<char, ThreadException::buffer_size>
  ThreadException::tl_message = {};
thread_local uint32_t ThreadException::tl_msg_sz = 0;

void ThreadException::initialize_exception(const char* format_str,
                                           std::va_list args)
{
  int res = std::vsprintf(tl_message.data(), format_str, args);

  if (res < 0) { // not enough space for formatted string
    std::memcpy(tl_message.data(), placeholder, placeholder_size);
    tl_msg_sz = placeholder_size;
  } else {
    tl_msg_sz = res;
  }
}

ThreadException::ThreadException(const char* format_str, ...)
{
  std::va_list args;
  va_start(args, format_str);
  initialize_exception(format_str, args);
}

const char* ThreadException::what() const noexcept
{
  // Not null ended till now
  tl_message[tl_msg_sz] = '\0';
  return tl_message.data();
}


SdlError::SdlError(const char* format_str, ...)
{
  std::va_list args;
  va_start(args, format_str);
  initialize_exception(format_str, args);
  add_sdl_error_message();
}

void SdlError::add_sdl_error_message()
{
  constexpr char separator[] = " [SDL Error] ";
  constexpr size_t separator_size = sizeof(separator) - 1;

  char* pos = tl_message.data() + tl_msg_sz;
  uint32_t rest = buffer_size - tl_msg_sz;

  if (separator_size <= rest) {
    std::memcpy(pos, separator, separator_size);
    rest -= separator_size;
    pos += separator_size;
  }

  const char* sdl_description = SDL_GetError();
  size_t sdl_description_size = strlen(sdl_description) - 1;
  if (sdl_description_size <= rest) {
    std::memcpy(pos, sdl_description, sdl_description_size);
    rest -= sdl_description_size;
    pos += sdl_description_size;
  }
}
