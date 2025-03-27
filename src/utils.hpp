// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_UTILS
#define SLOT_MACHINE_UTILS

#include <SDL3/SDL.h>
#include <array>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>

class ThreadException : public std::exception
{
public:
  ThreadException(const char* format_str, ...);
  virtual const char* what() const noexcept override;

private:
  static constexpr const char placeholder[] = "[Shorten message!]";
  static constexpr size_t placeholder_size = sizeof(placeholder) - 1;

protected:
  ThreadException() {};
  static void initialize_exception(const char* format_str, std::va_list args);

  static constexpr uint32_t buffer_size = 512;
  static thread_local std::array<char, buffer_size> tl_message;
  static thread_local uint32_t tl_msg_sz;
};


class SdlError : public ThreadException
{
public:
  SdlError(const char* format_str, ...);

private:
  static void add_sdl_error_message();
};
#endif
