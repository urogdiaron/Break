#pragma once
#include <vector>
#include <unordered_map>

#include <memory>
#include <SFML/Graphics.hpp>
#include <cstdint>

#include "util.h"
#include "../../Ecs/Project1/view.h"

#include "vec.h"
#include "collision.h"

#define BUILD_WITH_EASY_PROFILER
#define EASY_OPTION_START_LISTEN_ON_STARTUP 1
#define EASY_OPTION_LOG_ENABLED 1
#include "easy/profiler.h"

using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using uint = unsigned int;
using entityId = ecs::entityId;

struct Position : public vec
{
    using vec::Vector2;
};

struct Size : public vec
{
    using vec::Vector2;
};

struct Velocity : public vec
{
    using vec::Vector2;
};

struct Paddle
{
};

struct AttachedToPaddle
{
    entityId paddleId;
    vec relativePos;
};

struct Ball
{
};

struct Brick
{
    enum class Type
    {
        Simple,
        Ballspawner
    };
    Type type = Type::Simple;
    bool wasHitThisFrame = false;
};

struct GameState
{
};

struct BallCollision
{
    OverlapResult overlap;
    entityId ballId;
    entityId otherObjectId;
};

struct Tile
{
    std::vector<ecs::entityId> ids;
};

struct Globals
{
    ecs::Ecs ecs;
    GameState gameState;
    vec screenSize{ 800, 600 };
    sf::RenderWindow window{ sf::VideoMode{ (uint)screenSize.x, (uint)screenSize.y }, "Break" };

    sf::RectangleShape rectPrototype;
    sf::CircleShape circlePrototype;
    sf::RenderStates renderState;

    std::vector<BallCollision> ballCollisions;
    std::vector<Tile> tiles;
    float ballRespawnTimer = -1;
    float elapsedTime;
};