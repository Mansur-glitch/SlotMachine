// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "animation.hpp"

#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

std::pair<float, float> quad_equation(float a, float b, float c)
{
  std::pair<float, float> roots{ NAN, NAN };
  if (a == 0.f) {
    if (b == 0.f) {
      if (c == 0.f) {
        roots.first = std::numeric_limits<float>::infinity();
      }
    } else {
      roots.first = -c / b;
    }
    return roots;
  }

  float d = b * b - 4 * a * c;

  if (d > 0) {
    roots.first = (-b + std::sqrt(d)) / (2.f * a);
    roots.second = (-b - std::sqrt(d)) / (2.f * a);
  } else if (d == 0) {
    roots.first = -b / (2.f * a);
  }
  return roots;
};


UniformMotion::UniformMotion(float speed, float position) noexcept
  : Motion({ position, speed })
{
}


AcceleratedMotion::AcceleratedMotion(float acceleration,
                                     float speed,
                                     float position) noexcept
  : Motion({ position, speed, acceleration })
{
}

float AcceleratedMotion::nccry_acc_to_pos(float end_position, float time)
{
  assert(time != 0.f);
  return 2 * (end_position - components[pos] - components[spd] * time) /
         (time * time);
}

float AcceleratedMotion::nccry_acc_to_speed(float speed, float time)
{
  assert(time != 0.f);
  return (speed - components[spd]) / time;
}


JerkMotion::JerkMotion(float jerk,
                       float acceleration,
                       float speed,
                       float position) noexcept
  : Motion({ position, speed, acceleration, jerk })
{
}


ReelMotion::ReelMotion(float reel_length)
  : m_length(reel_length)
  , m_min_speed_proxy(0.f)
{
  assert(reel_length > 0.f);
}

void ReelMotion::set_reel_length(float length) noexcept
{
  assert(length > 0.f);
  m_length = length;
  if (get_position() >= m_length) {
    float rotations_made = std::floor(get_position() / m_length);
    set_positon(get_position() - rotations_made * m_length);
  }
}

void ReelMotion::stop_in(float end_position, float t)
{
  assert(t != 0.f);
  assert(end_position <= m_length);

  const float v = get_speed();
  const float s0 = get_position();
  const float s1 = end_position;
  const float l = m_length;

  float k; // minimum rotations required, so that speed doesn't go negative
  float a; // acceleration
  float j; // jerk

  // Used equation system to get formulas below
  // 1) k*l + s1 = s0 + v*t + a*t*t/2 + j*t*t*t/6
  //    Full path equation in jerk motion law. Need to use jerk motion because
  //    we specify 2 params: s1 - end position, t - finish time. We can't
  //    change speed because instant speed change looks unrealistic;
  //
  // 2) v + a*t + j*t*t/2 = 0
  //    Final speed equation in jerk motion law. Final speed equals zero: reel
  //    stop;
  //
  // 3) a + j*t_min = 0
  //    Previous formula differentiated. So we can calculate when minimal speed
  //    is reached;
  //
  // 4) t_min >= t
  //    Prevent speed go negative.

  k = std::ceil((v * t / 3.f + s0 - s1) / l);
  j = 12.f * (s0 - s1 - k * l + v * t / 2.f) / (t * t * t);
  a = -v / t - j * t / 2;

  set_jerk(j);
  set_acceleration(a);

  m_min_speed = 0.f;
  m_max_speed = float_limits::infinity();
  m_state = State::had_impact;
}

void ReelMotion::go_full_speed_in(float time)
{
  m_max_speed = m_max_speed_proxy;
  AcceleratedMotion accm{ get_acceleration(), get_speed(), get_position() };
  float new_acc = accm.nccry_acc_to_speed(m_max_speed, time);
  set_acceleration(new_acc);
  m_state = State::had_impact;
}

void ReelMotion::slow_to_minimal_in(float time)
{
  m_min_speed = m_min_speed_proxy;
  AcceleratedMotion accm{ get_acceleration(), get_speed(), get_position() };
  float new_acc = accm.nccry_acc_to_speed(m_min_speed, time);
  set_acceleration(new_acc);
}

void ReelMotion::set_min_speed(float speed)
{
  m_min_speed_proxy = speed;
}

void ReelMotion::set_max_speed(float speed)
{
  m_max_speed_proxy = speed;
}

void ReelMotion::advance(float dt)
{
  if (dt == 0.f) {
    return;
  }
  switch (m_state) {
    case State::had_impact:
      SpeedLimitedMotion::advance(dt);
      m_state = State::moving;
      break;
    case State::moving: {
      float stop_time = time_to_speed(0.f);
      if (stop_time <= dt) {
        SpeedLimitedMotion::advance(stop_time);
        full_stop();

      } else {
        SpeedLimitedMotion::advance(dt);
        if (get_max_speed() <= 0.f) { // ? may be rounding error
          full_stop();
        }
      }
      break;
    }
    default:
      break;
  }

  float rotations_made = std::floor(get_position() / m_length);
  set_positon(get_position() - rotations_made * m_length);
}

void ReelMotion::full_stop()
{
  set_speed(0.f);
  set_acceleration(0.f);
  set_jerk(0.f);
  set_positon(
    std::round(get_position())); // prevent rounding error accumulation
  m_state = State::rest;
}
