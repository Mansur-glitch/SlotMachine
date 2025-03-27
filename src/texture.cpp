// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "texture.hpp"
#include "graphics_system.hpp"
#include "utils.hpp"

#include <SDL3/SDL_iostream.h>
#include <SDL3_image/SDL_image.h>
#include <utility>

Texture Texture::from_surface(SDL_Surface* surface, GraphicsSystem& gs)
{
  Texture texture;
  texture.m_width = surface->w;
  texture.m_height = surface->h;

  texture.m_texture = SDL_CreateTextureFromSurface(gs.get_renderer(), surface);
  if (!texture.m_texture) {
    throw SdlError("Failed to create texture from surface");
  }

  return texture;
}

Texture Texture::from_svg_file(std::string file_name,
                               GraphicsSystem& gs,
                               uint16_t width,
                               uint16_t height)
{
  SDL_IOStream* svg_file = SDL_IOFromFile(file_name.c_str(), "rb");
  if (svg_file == nullptr) {
    throw SdlError("Failed to load image: %s", file_name.c_str());
  }

  SDL_Surface* surface = IMG_LoadSizedSVG_IO(svg_file, width, height);
  SDL_CloseIO(svg_file);
  if (!surface) {
    throw SdlError("%s image is not valid SVG format", file_name.c_str());
  }

  Texture result = Texture::from_surface(surface, gs);
  SDL_DestroySurface(surface);
  return result;
}

Texture::~Texture()
{
  if (m_texture) {
    SDL_DestroyTexture(m_texture);
    m_texture = nullptr;
    m_width = m_height = 0;
  }
}

void Texture::swap(Texture& other)
{
  std::swap(this->m_texture, other.m_texture);
  std::swap(this->m_width, other.m_width);
  std::swap(this->m_height, other.m_height);
}

Texture::Texture(Texture&& other) noexcept
{
  swap(other);
}

Texture& Texture::operator=(Texture&& other) noexcept
{
  swap(other);
  return *this;
}

TextureCollection::TextureCollection(std::string images_directory,
                                     uint32_t reserved)
{
  m_directory = images_directory + '/';
  if (reserved > 0) {
    m_textures.reserve(reserved);
    m_texture_ids.reserve(reserved);
  }
}

void swap(Texture& lhs, Texture& rhs)
{
  lhs.swap(rhs);
}


void TextureCollection::load(std::string file_name,
                             std::string texture_name,
                             GraphicsSystem& gs,
                             uint16_t w,
                             uint16_t h)
{
  if (file_name.substr(file_name.size() - 4) != ".svg") {
    throw ThreadException("Currently only .svg files supported");
  }
  if (auto it = m_texture_ids.find(texture_name); it != m_texture_ids.end()) {
    throw ThreadException("'%s' name is not unique", texture_name.c_str());
  }

  std::string file_path = m_directory + file_name;
  m_textures.push_back(Texture::from_svg_file(file_path, gs, w, h));
  m_texture_ids[texture_name] = m_textures.size(); // id = index + 1!
}

void TextureCollection::load_predefined(GraphicsSystem& gs)
{
  for (TextureResource res : big_resolution_textures) {
    load(res.file_name, res.tx_name, gs, 1024, 1024);
  }
  for (TextureResource res : surrounding_textures) {
    load(res.file_name, res.tx_name, gs);
  }
  for (TextureResource res : symbol_textures) {
    load(res.file_name, res.tx_name, gs);
  }
  for (TextureResource res : digit_textures) {
    load(res.file_name, res.tx_name, gs);
  }
}

TextureId TextureCollection::get_id(const std::string& texture_name) const
{
  if (auto it = m_texture_ids.find(texture_name); it == m_texture_ids.end()) {
    throw ThreadException("'%s' texture wasn't found", texture_name.c_str());
  } else {
    return it->second;
  }
}

Texture& TextureCollection::get_texture(TextureId id)
{
  assert(id <= m_textures.size());
  return m_textures[id - 1];
}

std::array<TextureId, 10> TextureCollection::get_digits() const
{
  char dg_tx_name[2] = { '0', '\0' };
  std::array<TextureId, 10> digit_tx_ids;
  for (uint32_t i = 0; i < 10; ++i) {
    dg_tx_name[0] = '0' + i;
    digit_tx_ids[i] = get_id(dg_tx_name);
  }
  return digit_tx_ids;
}
