#pragma once
#define BUILD_WITH_EASY_PROFILER
#define EASY_OPTION_START_LISTEN_ON_STARTUP 1
#define EASY_OPTION_LOG_ENABLED 1
#include "easy/profiler.h"


#include <vector>
#include <unordered_map>

#include <memory>
#include <SFML/Graphics.hpp>
#include <cstdint>

#include "util.h"
#include "../../Ecs/Project1/view.h"

#include "vec.h"
#include "collision.h"

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
    using vec::operator=;
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
};

struct CollidedWithBall
{
};

struct TileReferenceCreator
{
    bool isBall = false;
};

struct TileReference
{
    TileReference() = default;
    TileReference(bool isBall) : isBall(isBall) {}
    std::vector<std::pair<int, int>> tiles;
    bool isBall = false;
};

struct Visible
{
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

struct Camera
{
};

struct Particle 
{
    float timeToLive = 1.0f;
};

struct ParticleEmitter
{
    float timeUntilNextEmit = 0.05f;
};

struct Tile
{
    std::vector<ecs::entityId> ids;
};

struct Globals
{
    inline static float paddleHeight = 25.0f;
    inline static float ballRadius = 20.0f;
    inline static float ballStartingSpeed = 400.0f;
    inline static float tileSize = 50.0f;

    struct Prefabs
    {
        ecs::Prefab<Size, Position, Velocity, TileReferenceCreator, AttachedToPaddle, Ball, ParticleEmitter> attachedBall = { Size{ ballRadius * 2, ballRadius * 2 }, TileReferenceCreator{true} };
        ecs::Prefab<Size, Position, Velocity, TileReferenceCreator, Ball, ParticleEmitter> spawnedBall = { Size{ ballRadius * 2, ballRadius * 2 }, TileReferenceCreator{true} };
        ecs::Prefab<Position, Size, TileReferenceCreator, Paddle> paddle;
        ecs::Prefab<Position, Size, TileReferenceCreator, Brick> brick;
        ecs::Prefab<Position, Size, Camera> camera;
        ecs::Prefab<Position, Size, Particle, ecs::DontSaveEntity> particle = { Size{ 5.0f, 5.0f } };
    };

    ecs::Ecs ecs;
    GameState gameState;
    vec screenSize{ 800, 600 };
    sf::RenderWindow window{ sf::VideoMode{ (uint)screenSize.x, (uint)screenSize.y }, "Break" };

    sf::RectangleShape rectPrototype;
    sf::CircleShape circlePrototype;
    sf::RenderStates renderState;

    std::vector<BallCollision> ballCollisions;
    std::vector<Tile> tilesBalls;
    std::vector<Tile> tilesBricks;

    Prefabs prefabs;
    std::unordered_map<std::string, sf::Texture> textures;

    entityId camera;

    float ballRespawnTimer = -1;
    float elapsedTime;
};