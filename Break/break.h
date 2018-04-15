#pragma once
#include <vector>
#include <unordered_map>

#include <memory>
#include <SFML/Graphics.hpp>
#include <cstdint>

#include "util.h"

using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using uint = unsigned int;

using vec = sf::Vector2f;

struct Paddle
{
    // Center of the rectangle
    vec position;

    // Width of the rectangle
    float size = 150.0f;
};

struct Ball
{
    vec position;
    vec velocity;
    bool waitingToBeFired;
};

struct Brick
{
    vec position;
    vec size;
};

struct GameState
{
    Paddle paddle;
    Ball ball;
    std::vector<Brick> bricks;
};

struct Globals
{
    GameState gameState;
    vec screenSize{ 800, 600 };
    sf::RenderWindow window{ sf::VideoMode{ (uint)screenSize.x, (uint)screenSize.y }, "Break" };

    sf::RectangleShape rectPrototype;
    sf::CircleShape circlePrototype;
    sf::RenderStates renderState;

    sf::Clock clock;
};