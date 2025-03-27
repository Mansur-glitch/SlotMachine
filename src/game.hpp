// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_GAME
#define SLOT_MACHINE_GAME

#include "combination.hpp"
#include "scene.hpp"
#include "texture.hpp"

#include <list>
#include <memory>
#include <random>

class Game
{
public:
  using SymbolRow = Combination::SymbolRow;

  Game(TextureCollection& tc);
  void update(float dt);
  const Scene& get_scene() const noexcept { return m_scene; }
  void process_input(const SDL_Event& input_event);
  SymbolRow get_symbol_row();
  void set_symbol_row(const SymbolRow& row); // for tests

private:
  enum class Event
  {
    start_pressed = 0,
    stop_pressed,
    enable_stop_timer,
    spin_time_out,
    reels_stopped,
    show_result_time_out,
    number
  };

  class State;
  class IdleState;
  class SpeedUpState;
  class StopWaitState;
  class SlowingDownState;
  class ResultState;

  class Timer;
  struct PostponedEvent;

  void add_timer_event(FloatSeconds time, Event e);
  void handle_event(Event e);
  void check_timers();
  // Highlight combo sybmols
  void highlight_combo(Combination::Range r);
  // Remove combo highlight
  void remove_highlight();

  std::mt19937 m_rng;
  std::list<PostponedEvent> m_timer_events;
  Scene m_scene;
  SlotMachine& m_machine;
  std::unique_ptr<State> m_state;
};


class Game::State
{
public:
  State(Game& g)
    : m_game(g) {};
  virtual ~State() {};
  virtual std::unique_ptr<State> next(Event game_event) = 0;

protected:
  Game& m_game;
};


class Game::IdleState : public State
{
public:
  IdleState(Game& g);
  virtual std::unique_ptr<State> next(Event game_event) override;
};


class Game::SpeedUpState : public State
{
public:
  SpeedUpState(Game& g);
  virtual std::unique_ptr<State> next(Event game_event) override;
};


class Game::StopWaitState : public State
{
public:
  StopWaitState(Game& g);
  virtual std::unique_ptr<State> next(Event game_event) override;
};


class Game::SlowingDownState : public State
{
public:
  SlowingDownState(Game& g);
  virtual std::unique_ptr<State> next(Event game_event) override;

private:
  SymbolRow m_stop_row;
};


class Game::ResultState : public State
{
public:
  ResultState(Game& g, SymbolRow stop_row);
  virtual std::unique_ptr<State> next(Event game_event) override;

private:
  SymbolRow m_stop_row;
  bool m_auto_spin;
};


class Game::Timer
{
public:
  Timer(FloatSeconds period);
  bool is_expired();

private:
  TimePoint m_expire_time;
};


struct Game::PostponedEvent
{
  Timer timer;
  std::function<void()> event_handler;
};
#endif
