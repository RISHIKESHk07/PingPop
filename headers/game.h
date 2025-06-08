#pragma once
#include "iostream"
#include "vector"
#include <SFML/Window.hpp>
class GameState {
public:
  GameState();
  virtual ~GameState();
  void eventUpdate();
  void render();

private:
  std::vector<GameState> states;
};
