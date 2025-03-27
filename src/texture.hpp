// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_TEXTURE
#define SLOT_MACHINE_TEXTURE

#include "symbol.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_surface.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>


using TextureId = uint32_t;
static constexpr TextureId NULL_TEXTURE = { 0 };

class GraphicsSystem;
class Texture
{
public:
  static Texture from_surface(SDL_Surface* surface, GraphicsSystem& gs);
  static Texture from_svg_file(std::string file_name,
                               GraphicsSystem& gs,
                               uint16_t width,
                               uint16_t height);

  ~Texture();
  void swap(Texture& other);
  Texture(Texture&& other) noexcept;
  Texture& operator=(Texture&& other) noexcept;

  uint16_t get_width() const noexcept { return m_width; }
  uint16_t get_height() const noexcept { return m_height; }
  SDL_Texture* get_handler() const noexcept { return m_texture; }

private:
  Texture() = default;
  Texture(const Texture& other) = delete;
  Texture& operator=(const Texture& other) = delete;

  SDL_Texture* m_texture{ nullptr };
  uint16_t m_width{ 0 };
  uint16_t m_height{ 0 };
};

void swap(Texture& lhs, Texture& rhs);


struct TextureResource
{
  const char* file_name;
  const char* tx_name;
};


class TextureCollection
{
public:
  TextureCollection(std::string images_directory, uint32_t reserved = 0);
  void load(std::string file_name,
            std::string texture_name,
            GraphicsSystem& gs,
            uint16_t w = 256,
            uint16_t h = 256);
  void load_predefined(GraphicsSystem& gs);
  TextureId get_id(const std::string& texture_name) const;
  Texture& get_texture(TextureId id);
  std::array<TextureId, 10> get_digits() const;

private:
  // Predefined texture resources
  static constexpr std::array big_resolution_textures = {
    TextureResource{ "background.svg", "background" },
  };
  static constexpr std::array surrounding_textures = {
    TextureResource{ "start_white.svg", "start_enabled" },
    TextureResource{ "start_grey.svg", "start_disabled" },
    TextureResource{ "stop_white.svg", "stop_enabled" },
    TextureResource{ "stop_grey.svg", "stop_disabled" },
  };
  static constexpr std::array digit_textures = {
    TextureResource{ "0.svg", "0" }, TextureResource{ "1.svg", "1" },
    TextureResource{ "2.svg", "2" }, TextureResource{ "3.svg", "3" },
    TextureResource{ "4.svg", "4" }, TextureResource{ "5.svg", "5" },
    TextureResource{ "6.svg", "6" }, TextureResource{ "7.svg", "7" },
    TextureResource{ "8.svg", "8" }, TextureResource{ "9.svg", "9" },
  };
  static constexpr std::array symbol_textures = {
    TextureResource{ "lucky_seven.svg", get_symbol_name(Symbol::lucky_seven) },
    TextureResource{ "cross.svg", get_symbol_name(Symbol::cross) },
    TextureResource{ "question.svg", get_symbol_name(Symbol::question) },
    TextureResource{ "respin.svg", get_symbol_name(Symbol::respin) },
    TextureResource{ "apple.svg", get_symbol_name(Symbol::apple) },
    TextureResource{ "carrot.svg", get_symbol_name(Symbol::carrot) },
    TextureResource{ "corn.svg", get_symbol_name(Symbol::corn) },
    TextureResource{ "grape.svg", get_symbol_name(Symbol::grape) },
    TextureResource{ "spade.svg", get_symbol_name(Symbol::spade) },
    TextureResource{ "club.svg", get_symbol_name(Symbol::club) },
    TextureResource{ "heart.svg", get_symbol_name(Symbol::heart) },
    TextureResource{ "diamond.svg", get_symbol_name(Symbol::diamond) },
    TextureResource{ "amethyst.svg", get_symbol_name(Symbol::amethyst) },
    TextureResource{ "emerald.svg", get_symbol_name(Symbol::emerald) },
    TextureResource{ "topaz.svg", get_symbol_name(Symbol::topaz) },
    TextureResource{ "crystal.svg", get_symbol_name(Symbol::crystal) },
  };
  static_assert(symbol_textures.size() == g_nsymbols);

  std::string m_directory;
  std::vector<Texture> m_textures;
  std::unordered_map<std::string, TextureId> m_texture_ids;
};
#endif
