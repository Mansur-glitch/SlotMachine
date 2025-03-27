// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_MOTION
#define SLOT_MACHINE_MOTION

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

std::pair<float, float> quad_equation(float a, float b, float c);


enum class MotionType : uint8_t
{
  stationary = 0, // doesnt move
  uniform,        // move with constant speed
  accelerated,    // move with constant acceleration
  jerked,         // acceleration changes linearly
  number
};

constexpr bool operator>=(MotionType lhs, MotionType rhs)
{
  return static_cast<uint8_t>(lhs) >= static_cast<uint8_t>(rhs);
}


template<MotionType MT>
class Motion
{
public:
  using float_limits = std::numeric_limits<float>;

  enum class MotionComponent : uint8_t
  {
    positon = 0,
    speed,
    acceleration,
    jerk,
    number
  };
  static constexpr MotionComponent pos = MotionComponent::positon;
  static constexpr MotionComponent spd = MotionComponent::speed;
  static constexpr MotionComponent acc = MotionComponent::acceleration;
  static constexpr MotionComponent jrk = MotionComponent::jerk;

  static constexpr uint8_t n_components = static_cast<uint8_t>(MT) + 1;
  struct ComponentArray
  {
    const float& operator[](MotionComponent t) const noexcept
    {
      return data[static_cast<uint8_t>(t)];
    }
    float& operator[](MotionComponent t) noexcept
    {
      return data[static_cast<uint8_t>(t)];
    }

    std::array<float, n_components> data;
  };

  Motion(std::array<float, n_components> cmp) noexcept
  {
    for (uint8_t i = 0; i < n_components; ++i) {
      components.data[i] = cmp[i];
    }
  }

  void advance(float dt) noexcept
  {
    const float j = get_jerk();
    const float a = get_acceleration();
    const float v = get_speed();
    const float s = get_position();

    set_positon(s + (v + (a / 2.f + j * dt / 6.f) * dt) * dt);
    set_speed(v + (a + j * dt / 2.f) * dt);
    set_acceleration(a + j * dt);
  }

  float time_to_speed(float v1, float* second_res = nullptr) const noexcept
  {

    if (second_res != nullptr) {
      *second_res = float_limits::infinity();
    }

    const float j = get_jerk();
    const float a = get_acceleration();
    const float v0 = get_speed();

    if constexpr (MT == MotionType::stationary) {
      return v1 == 0.f ? 0.f : float_limits::infinity();

    } else if constexpr (MT == MotionType::uniform) {
      return v1 == v0 ? 0.f : float_limits::infinity();

    } else if constexpr (MT == MotionType::accelerated) {
      float t = (v1 - v0) / a;
      if (t < 0.f) {
        return float_limits::infinity();
      }
      return t;

    } else {
      auto roots = quad_equation(j / 2.f, a, v0 - v1);
      if (std::isnan(roots.first)) {
        return float_limits::infinity();
      }
      if (std::isnan(roots.second)) {
        if (roots.first < 0.f) {
          return float_limits::infinity();
        }
        return roots.first;
      }
      float min_root = std::min(roots.first, roots.second);
      float max_root = std::max(roots.first, roots.second);
      if (max_root < 0.f) {
        return float_limits::infinity();
      }
      if (min_root < 0.f) {
        return max_root;
      }

      if (second_res != nullptr) {
        *second_res = max_root;
      };
      return min_root;
    }
  }

  float get_position() const noexcept { return components[pos]; }
  void set_positon(float p) noexcept { components[pos] = p; }

  float get_speed() const noexcept
  {
    if constexpr (MT >= MotionType::uniform) {
      return components[spd];
    }
    return 0.f;
  }

  void set_speed(float s) noexcept
  {
    if constexpr (MT >= MotionType::uniform) {
      components[spd] = s;
    }
  }

  float get_acceleration() const noexcept
  {
    if constexpr (MT >= MotionType::accelerated) {
      return components[acc];
    }
    return 0.f;
  }

  void set_acceleration(float a) noexcept
  {
    if constexpr (MT >= MotionType::accelerated) {
      components[acc] = a;
    }
  }

  float get_jerk() const noexcept
  {
    if constexpr (MT >= MotionType::jerked) {
      return components[jrk];
    }
    return 0.f;
  }

  void set_jerk(float j) noexcept
  {
    if constexpr (MT >= MotionType::jerked) {
      components[jrk] = j;
    }
  }

protected:
  ComponentArray components{};
};


class StaticMotion : public Motion<MotionType::stationary>
{
public:
  StaticMotion(float position = 0.f) noexcept;

private:
  using Motion::get_acceleration;
  using Motion::get_jerk;
  using Motion::get_speed;
  using Motion::set_acceleration;
  using Motion::set_jerk;
  using Motion::set_speed;
};


class UniformMotion : public Motion<MotionType::uniform>
{
public:
  UniformMotion(float speed = 0.f, float position = 0.f) noexcept;

private:
  using Motion::get_acceleration;
  using Motion::get_jerk;
  using Motion::set_acceleration;
  using Motion::set_jerk;
};


class AcceleratedMotion : public Motion<MotionType::accelerated>
{
public:
  AcceleratedMotion(float acceleration = 0.f,
                    float speed = 0.f,
                    float position = 0.f) noexcept;
  float nccry_acc_to_pos(float positon, float time);
  float nccry_acc_to_speed(float speed, float time);

private:
  using Motion::get_jerk;
  using Motion::set_jerk;
};


class JerkMotion : public Motion<MotionType::jerked>
{
public:
  JerkMotion(float jerk = 0.f,
             float acceleration = 0.f,
             float speed = 0.f,
             float positon = 0.f) noexcept;
};


template<MotionType MT>
class SpeedLimitedMotion : public Motion<MT>
{
  static_assert(MT >= MotionType::accelerated);

public:
  using float_limits = std::numeric_limits<float>;

  SpeedLimitedMotion(
    float min_speed = 0.f,
    float max_speed = 0.f,
    std::array<float, Motion<MT>::n_components> cmps = {}) noexcept
    : Motion<MT>(cmps)
    , m_min_speed(min_speed)
    , m_max_speed(max_speed)
  {
  }

  bool limit_reached() const noexcept
  {
    return this->get_speed() <= m_min_speed || this->get_speed() >= m_max_speed;
  }

  // Keeps speed the same
  void uniform_advance(float dt) noexcept
  {
    this->set_positon(this->get_position() + this->get_speed() * dt);
    this->set_acceleration(this->get_acceleration() + this->get_jerk() * dt);
  }

  void advance(float dt)
  {
    struct ExtremumHit
    {
      float time;
      float value;
      bool operator<(const ExtremumHit& other) const
      {
        return time < other.time;
      }
    };

    // In jerk motion speed changes according to quadratic law (parabola graph)
    // While moving we can hit minimum and maximum values up to twice each
    float time_to_max2 = float_limits::infinity();
    float time_to_max1 = this->time_to_speed(m_max_speed, &time_to_max2);
    float time_to_min2 = float_limits::infinity();
    float time_to_min1 = this->time_to_speed(m_min_speed, &time_to_min2);

    std::array extremums = { ExtremumHit{ time_to_max1, m_max_speed },
                             ExtremumHit{ time_to_max2, m_max_speed },
                             ExtremumHit{ time_to_min1, m_min_speed },
                             ExtremumHit{ time_to_min2, m_min_speed } };
    std::sort(extremums.begin(), extremums.end());

    float prev_t = 0.f;
    for (int i = 0; i < extremums.size() && !std::isinf(extremums[i].time);
         ++i) {
      bool is_time_up = dt <= extremums[i].time;
      float t_spent = (is_time_up ? dt : extremums[i].time) - prev_t;

      bool is_exceeded;
      bool naturaly_limited;
      if (extremums[i].value == m_max_speed) {
        is_exceeded = this->get_speed() >= m_max_speed;
        naturaly_limited = this->get_jerk() > 0.f; // Finite parabola bottom
      } else {
        is_exceeded = this->get_speed() <= m_min_speed;
        naturaly_limited = this->get_jerk() < 0.f; // Finite parabola top
      }
      // If currently at the top/bottom part of the parabola
      bool is_prev_same = i > 0 && extremums[i - 1].value == extremums[i].value;
      // Traversing top/bottom of parabola that under our limit
      bool will_reduce = is_prev_same && naturaly_limited;

      if (is_exceeded && !will_reduce) {
        uniform_advance(t_spent);
      } else {
        Motion<MT>::advance(t_spent);
        if (!is_time_up) {                     // reached extremum
          this->set_speed(extremums[i].value); // ensure no rounding error
        }
      }

      if (is_time_up) {
        prev_t = dt;
        break;
      }
      prev_t = extremums[i].time;
    }
    if (prev_t < dt) { // right branch of the parabola
      uniform_advance(dt - prev_t);
    }
  }

  float get_max_speed() const noexcept { return m_max_speed; }
  void set_max_speed(float speed) noexcept { m_max_speed = speed; }
  float get_min_speed() const noexcept { return m_min_speed; }
  void set_min_speed(float speed) noexcept { m_min_speed = speed; }

protected:
  float m_max_speed;
  float m_min_speed;
};


class ReelMotion : public SpeedLimitedMotion<MotionType::jerked>
{
public:
  ReelMotion(float reel_length);
  float get_reel_length() const noexcept { return m_length; }
  void set_reel_length(float length) noexcept;
  // Will stop reel at specified time in specified position regardless min_speed
  // If starts from rest may exceed max speed limit to get to position in time
  void stop_in(float end_position, float time);
  void go_full_speed_in(float time);
  void slow_to_minimal_in(float time);

  // Hide parent's definitions
  void advance(float dt);
  void set_min_speed(float speed);
  void set_max_speed(float speed);

private:
  enum class State
  {
    rest = 0,
    had_impact, // allow to move from rest state
    moving,     // may stop any moment
    number
  };
  void full_stop();

  State m_state{ State::rest };
  float m_length;
  // User preffered speed limits
  float m_min_speed_proxy{ 0.f };
  float m_max_speed_proxy{ 0.f };
};
#endif
