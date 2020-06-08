#include "break.h"
#include <iostream>
#include <fstream>

Globals g_Globals;

bool g_debugBall = false;

void render_line(vec v1, vec v2, sf::Color color = sf::Color::White)
{
    v1.y = g_Globals.screenSize.y - v1.y;
    v2.y = g_Globals.screenSize.y - v2.y;


    sf::Vertex vertices[] = { sf::Vertex{v1, color}, sf::Vertex{ v2, color } };
    g_Globals.window.draw(vertices, 2, sf::Lines);
}

void render_rect(vec center, vec dimensions, sf::Color color = sf::Color::White)
{
    center.y = g_Globals.screenSize.y - center.y;

    sf::RenderStates renderState;
    renderState.transform.translate(center).scale(dimensions);
    g_Globals.rectPrototype.setFillColor(color);
    g_Globals.window.draw(g_Globals.rectPrototype, renderState);
}

void render_circle(vec center, float radius, sf::Color color = sf::Color::White)
{
    center.y = g_Globals.screenSize.y - center.y;
    center.x -= radius;
    center.y -= radius;
    sf::RenderStates renderState;
    renderState.transform.translate(center).scale(radius, radius);
    g_Globals.circlePrototype.setFillColor(color);
    g_Globals.window.draw(g_Globals.circlePrototype, renderState);
}

ecs::Ecs& getEcs()
{
    return g_Globals.ecs;
}

ecs::Scheduler& getScheduler()
{
    return g_Globals.scheduler;
}

int get_tile_count_x()
{
    int tileCountX = (int)(g_Globals.screenSize.x / Globals::tileSize);
    return tileCountX;
}

int get_tile_count_y()
{
    int tileCountY = (int)(g_Globals.screenSize.y / Globals::tileSize);
    return tileCountY;
}

void init_tiles()
{
    int tileCountX = get_tile_count_x();
    int tileCountY = get_tile_count_y();

    g_Globals.tilesBricks = std::vector<Tile>(tileCountX * tileCountY);
    g_Globals.tilesBalls = std::vector<Tile>(tileCountX * tileCountY);
}

void register_types(){
    auto& ecs = getEcs();
    ecs.registerType<Position>("Position");
    ecs.registerType<Size>("Size");
    ecs.registerType<Velocity>("Velocity");
    ecs.registerType<TileReferenceCreator>("TileReferenceCreator");
    ecs.registerType<TileReference>("TileReference", ecs::ComponentType::State);
    ecs.registerType<Paddle>("Paddle");
    ecs.registerType<AttachedToPaddle>("AttachedToPaddle");
    ecs.registerType<Ball>("Ball");
    ecs.registerType<Brick>("Brick");
    ecs.registerType<CollidedWithBall>("CollidedWithBall");
    ecs.registerType<Camera>("Camera");
    ecs.registerType<Visibility>("Visibility", ecs::ComponentType::Shared);     // the exact value doesnt need to be saved but the existence of this should be
    ecs.registerType<Particle>("Particle");
    ecs.registerType<ParticleEmitter>("ParticleEmitter");
    ecs.registerType<TransformOrder>("TransformOrder", ecs::ComponentType::Shared);
    ecs.registerType<Sprite>("SpriteIndex", ecs::ComponentType::Shared);
}

int loadTexture(const char* fileName)
{
    sf::Texture texture;
    bool success = texture.loadFromFile(fileName);
    if (!success)
        return 0;

    int textureIndex = (int)g_Globals.textures.size() + 1;
    g_Globals.textures[textureIndex] = texture;
    return textureIndex;
}

void loadTextures()
{
    sf::Texture texture;
    loadTexture("brick.png");
    loadTexture("paddle.png");
    loadTexture("ball.png");
}

void init_globals()
{
    //g_Globals.gameState  paddle.position.y = Globals::paddleHeight * 0.5f;
    g_Globals.rectPrototype.setSize(vec{ 1.0f, 1.0f });
    g_Globals.rectPrototype.setOrigin(vec{0.5f, 0.5f});
    g_Globals.rectPrototype.setFillColor(sf::Color::White);

    g_Globals.circlePrototype.setRadius(1.0f);
    g_Globals.circlePrototype.setFillColor(sf::Color(100, 100, 100));

    register_types();
    loadTextures();
    init_tiles();
}

void insert_into_tiles(ecs::entityId id, const Position& pos, const Size& size, TileReference& tileRef)
{
    vec min = pos - size * 0.5f;
    vec max = pos + size * 0.5f;

    int firstTileX = (int)(min.x / Globals::tileSize);
    int lastTileX = (int)(max.x / Globals::tileSize);

    int firstTileY = (int)(min.y / Globals::tileSize);
    int lastTileY = (int)(max.y / Globals::tileSize);

    int tileCountX = get_tile_count_x();
    int tileCountY = get_tile_count_y();

    auto& tiles = tileRef.isBall ? g_Globals.tilesBalls : g_Globals.tilesBricks;

    for (int y = firstTileY; y <= lastTileY; y++)
    {
        if (y < 0 || y >= tileCountY)
            continue;

        for (int x = firstTileX; x <= lastTileX; x++)
        {
            if (x < 0 || x >= tileCountX)
                continue;

            g_Globals.tilesMtx.lock();
            auto& tile = tiles[y * tileCountX + x];
            g_Globals.tilesMtx.unlock();

            {
#ifdef LOCK_TILES
                std::lock_guard<std::mutex> lock(tile.mtx);
#endif
                tile.ids.push_back(id);
            }
            tileRef.tiles.push_back(std::make_pair(x, y));
        }
    }
}

void update_tiles_after_move(ecs::entityId id, const Position& oldPos, const Position& newPos, const Size& size, TileReference& tileRef)
{
    EASY_FUNCTION();
    vec oldMin = oldPos - size * 0.5f;
    vec oldMax = oldPos + size * 0.5f;

    int oldFirstTileX = (int)(oldMin.x / Globals::tileSize);
    int oldLastTileX = (int)(oldMax.x / Globals::tileSize);

    int oldFirstTileY = (int)(oldMin.y / Globals::tileSize);
    int oldLastTileY = (int)(oldMax.y / Globals::tileSize);

    vec newMin = newPos - size * 0.5f;
    vec newMax = newPos + size * 0.5f;

    int newFirstTileX = (int)(newMin.x / Globals::tileSize);
    int newLastTileX = (int)(newMax.x / Globals::tileSize);

    int newFirstTileY = (int)(newMin.y / Globals::tileSize);
    int newLastTileY = (int)(newMax.y / Globals::tileSize);

    int tileCountX = get_tile_count_x();
    int tileCountY = get_tile_count_y();

    auto& tiles = tileRef.isBall ? g_Globals.tilesBalls : g_Globals.tilesBricks;

    for (int y = oldFirstTileY; y <= oldLastTileY; y++)
    {
        if (y < 0 || y >= tileCountY)
            continue;

        bool isOutsideY = (y < newFirstTileY || y > newLastTileY);
        for (int x = oldFirstTileX; x <= oldLastTileX; x++)
        {
            if (x < 0 || x >= tileCountX)
                continue;

            bool isOutsideX = (x < newFirstTileX || x > newLastTileX);
            if (isOutsideX || isOutsideY)
            {
                g_Globals.tilesMtx.lock();
                auto& tile = tiles[y * tileCountX + x];
                g_Globals.tilesMtx.unlock();

                {
#ifdef LOCK_TILES
                    std::lock_guard<std::mutex> lock(tile.mtx);
#endif
                    auto it = std::find(tile.ids.begin(), tile.ids.end(), id);
                    if (it != tile.ids.end())
                    {
                        unordered_delete(tile.ids, it);
                        auto itTileRef = std::find(tileRef.tiles.begin(), tileRef.tiles.end(), std::make_pair(x, y));
                        if (itTileRef != tileRef.tiles.end())
                            unordered_delete(tileRef.tiles, itTileRef);
                    }
                }
            }
        }
    }

    for (int y = newFirstTileY; y <= newLastTileY; y++)
    {
        if (y < 0 || y >= tileCountY)
            continue;

        bool isOutsideY = (y < oldFirstTileY || y > oldLastTileY);
        for (int x = newFirstTileX; x <= newLastTileX; x++)
        {
            if (x < 0 || x >= tileCountX)
                continue;

            bool isOutsideX = (x < oldFirstTileX || x > oldLastTileX);
            if (isOutsideX || isOutsideY)
            {
                g_Globals.tilesMtx.lock();
                auto& tile = tiles[y * tileCountX + x];
                g_Globals.tilesMtx.unlock();

                {
#ifdef LOCK_TILES
                    std::lock_guard<std::mutex> lock(tile.mtx);
#endif
                    auto it = std::find(tile.ids.begin(), tile.ids.end(), id);
                    if (it == tile.ids.end())
                    {
                        tile.ids.push_back(id);
                        tileRef.tiles.push_back(std::make_pair(x, y));
                    }
                }
            }
        }
    }
}

void update_tiles_for_deletion(ecs::entityId id, const TileReference& tileRef)
{
    int tileCountX = get_tile_count_x();
    auto& tiles = tileRef.isBall ? g_Globals.tilesBalls : g_Globals.tilesBricks;

    for (auto [x, y] : tileRef.tiles)
    {
        g_Globals.tilesMtx.lock();
        auto& tile = tiles[y * tileCountX + x];
        g_Globals.tilesMtx.unlock();

        {
#ifdef LOCK_TILES
            std::lock_guard<std::mutex> lock(tile.mtx);
#endif
            auto it = std::find(tile.ids.begin(), tile.ids.end(), id);
            if (it != tile.ids.end())
            {
                unordered_delete(tile.ids, it);
            }
        }
    }
}

struct CreateTileReferencesForNewEntities : public ecs::System
{
    void scheduleJobs() override
    {
        auto createTileReferences = ecs::Job(ecs->view<Position, Size, TileReferenceCreator>().exclude<TileReference>());
        JOB_SET_FN(createTileReferences)
        {
            auto& [id, pos, size, tileReferenceCreator] = *it;
            TileReference tileRef{ tileReferenceCreator.isBall };
            insert_into_tiles(id, pos, size, tileRef);
            it.getView()->addComponent<TileReference>(id, tileRef);
        };
        JOB_SCHEDULE(createTileReferences);
    }
};

void clear_tile_references_for_deleted_entities()
{
    EASY_FUNCTION();
    auto& ecs = getEcs();
    auto v = ecs.view<TileReference>().exclude<TileReferenceCreator>();
    for (auto& [id, tileReference] : v)
    {
        update_tiles_for_deletion(id, tileReference);
        v.deleteComponents<TileReference>(id);
    }
    ecs.executeCommmandBuffer();
}

void setup_level(GameState& gamestate)
{
    auto& ecs = getEcs();

    // Create the paddle
    auto pos = Position{ g_Globals.screenSize.x * 0.5f, Globals::paddleHeight * 0.5f };
    auto size = Size{ 150.0f, Globals::paddleHeight };
    entityId paddleId = ecs.createEntity(g_Globals.prefabs.paddle, pos, size);

    int rows = 50;
    int columns = 50;

    float brickSpacing = 0;

	float levelMarginHorizontal = 50.0f;
	float levelMarginTop = 30.0f;
	float levelMarginBottom = 200.0f;

    vec brickSize = vec{ 
		(g_Globals.screenSize.x - levelMarginHorizontal * 2 - (columns - 1) * brickSpacing) / columns,
		(g_Globals.screenSize.y - levelMarginBottom - levelMarginTop - (rows - 1) * brickSpacing) / rows};

	vec currentBrickPos{ levelMarginHorizontal + brickSize.x * 0.5f, g_Globals.screenSize.y - levelMarginTop - brickSize.y * 0.5f };
	vec originalBrickPosition = currentBrickPos; // for resetting after new line

	for (int j = 0; j < rows; j++)
    {
		for (int i = 0; i < columns; i++)
		{
            Brick::Type brickType = Brick::Type::Simple;
            int random = rand() % 1;
            if (random == 0)
            {
                brickType = Brick::Type::Ballspawner;
            }

            ecs::entityId id = ecs.createEntity(g_Globals.prefabs.brick,
                Position{ currentBrickPos }, Size{ brickSize }, Brick{ brickType },
                Sprite{ 1, brickType == Brick::Type::Ballspawner ? sf::Color(255, 255, 0, 255) : sf::Color(255, 50, 10, 255) }
            );
            currentBrickPos.x += brickSize.x + brickSpacing;
        }
        currentBrickPos.x = originalBrickPosition.x;
        currentBrickPos.y -= brickSize.y + brickSpacing;
    }

    g_Globals.camera = ecs.createEntity(g_Globals.prefabs.camera, Position{ g_Globals.screenSize * 0.5f }, Size{ g_Globals.screenSize });
}

void spawn_ball_on_paddle()
{
    auto& ecs = getEcs();
    auto paddleView = ecs.view<Position, Size>().with<Paddle>();
    auto itPaddle = paddleView.begin();
    if (itPaddle == paddleView.end())
        return;

    auto& [paddleId, pos, size] = *itPaddle;
    vec ballPosition = pos + vec(0.0f, size.y * 0.5f + Globals::ballRadius);

    ecs::entityId ballId = ecs.createEntity(g_Globals.prefabs.attachedBall,
        Position{ ballPosition },
        AttachedToPaddle{ paddleId, vec(0.0f, size.y * 0.5f + Globals::ballRadius)
    });
    getEcs().executeCommmandBuffer();
}

void fire_ball()
{
    auto view = getEcs().view<Velocity>().with<Ball, AttachedToPaddle>();
    for (auto& [id, vel] : view)
    {
        (vec&)vel = vec_normalize(vec{ 1,1 }) * Globals::ballStartingSpeed;
        view.deleteComponents<AttachedToPaddle>(id);
    }
}

void update_paddle_by_mouse()
{
    EASY_FUNCTION();
    vec mousePosition{ sf::Mouse::getPosition(g_Globals.window) };
    float screenSizeX = g_Globals.screenSize.x;
    for (auto& [id, pos, size, tileRef] : getEcs().view<Position, Size, TileReference>().with<Paddle>())
    {
        auto oldPos = pos;
        pos.x = mousePosition.x;
        pos.x = std::max(pos.x, size.x * 0.5f);
        pos.x = std::min(pos.x, screenSizeX - size.x * 0.5f);
        update_tiles_after_move(id, oldPos, pos, size, tileRef);
    }
}

void update_balls_attached_to_paddle()
{
    EASY_FUNCTION();
    auto& ecs = getEcs();
    for (auto& [id, pos, tileRef, attach] : ecs.view<Position, TileReference, const AttachedToPaddle>())
    {
        auto paddlePos = ecs.getComponent<Position>(attach.paddleId);
        if (!paddlePos)
            continue;
        auto oldPos = pos;
        pos = *paddlePos + attach.relativePos;
        update_tiles_after_move(id, oldPos, pos, Size(Globals::ballRadius, Globals::ballRadius), tileRef);
    }
}

struct TiledPositionByVelocitySystem : public ecs::System
{
    void scheduleJobs() override
    {
        float dt = g_Globals.elapsedTime;

        auto updateTiledPositions = ecs::Job(ecs->view<Position, const Velocity, const Size, TileReference>().exclude<AttachedToPaddle>());
        JOB_SET_FN(updateTiledPositions)
        {
            auto& [id, pos, vel, size, tileRef] = *it;
            if (vel.x == 0.0f && vel.y == 0.0)
                return;

            auto oldPos = pos;
            pos += vel * dt;
            update_tiles_after_move(id, oldPos, pos, Size(Globals::ballRadius, Globals::ballRadius), tileRef);
        };
        JOB_SCHEDULE(updateTiledPositions);
    }
};

struct SimplePositionByVelocitySystem : public ecs::System
{
    void scheduleJobs() override
    {
        float dt = g_Globals.elapsedTime;
        auto updateSimple = ecs::Job(ecs->view<Position, const Velocity>().exclude<AttachedToPaddle, TileReference>());
        JOB_SET_FN(updateSimple)
        {
            auto& [id, pos, vel] = *it;
            pos += vel * dt;
        };
        JOB_SCHEDULE(updateSimple);
    }

};

struct UpdateBallCollisionsTiled : public ecs::System
{
    void scheduleJobs() override
    {
        int tileCountX = get_tile_count_x();
        int tileCountY = get_tile_count_y();

        g_Globals.ballCollisions.clear();

        auto updateBallCollisions = ecs::Job(ecs->view<Position, TileReference>().with<Ball>());
        JOB_SET_FN(updateBallCollisions)
        {
            auto& [ballId, ballPos, ballTileRef] = *it;

            bool leftEdge = false, rightEdge = false, topEdge = false, bottomEdge = false;
            size_t tileCount = ballTileRef.tiles.size();
            for (auto [x, y] : ballTileRef.tiles)
            {
                if (x == 0)
                    leftEdge = true;
                if (x == tileCountX - 1)
                    rightEdge = true;
                if (y == 0)
                    bottomEdge = true;
                if (y == tileCountY - 1)
                    topEdge = true;
            }

            if (!tileCount || (bottomEdge && ballPos.y < 0))
            {   // Ball dies
                it.getView()->deleteEntity(ballId);
                return;
            }

            if (leftEdge && ballPos.x - Globals::ballRadius < 0)
            {
                BallCollision ballCollision;
                ballCollision.ballId = ballId;
                ballCollision.otherObjectId = 0;
                ballCollision.overlap.penetration = -(ballPos.x - Globals::ballRadius);
                ballCollision.overlap.collisionPoint = vec(0, ballPos.y);
                ballCollision.overlap.normal = vec(1, 0);
                g_Globals.ballCollisionMtx.lock();
                g_Globals.ballCollisions.push_back(ballCollision);
                g_Globals.ballCollisionMtx.unlock();
            }
            if (rightEdge && ballPos.x + Globals::ballRadius > g_Globals.screenSize.x)
            {
                BallCollision ballCollision;
                ballCollision.ballId = ballId;
                ballCollision.otherObjectId = 0;
                ballCollision.overlap.penetration = ballPos.x + Globals::ballRadius - g_Globals.screenSize.x;
                ballCollision.overlap.collisionPoint = vec(g_Globals.screenSize.x, ballPos.y);
                ballCollision.overlap.normal = vec(-1, 0);
                g_Globals.ballCollisionMtx.lock();
                g_Globals.ballCollisions.push_back(ballCollision);
                g_Globals.ballCollisionMtx.unlock();
            }
            if (topEdge && ballPos.y + Globals::ballRadius > g_Globals.screenSize.y)
            {
                BallCollision ballCollision;
                ballCollision.ballId = ballId;
                ballCollision.otherObjectId = 0;
                ballCollision.overlap.penetration = ballPos.y + Globals::ballRadius - g_Globals.screenSize.y;
                ballCollision.overlap.collisionPoint = vec(ballPos.x, g_Globals.screenSize.y);
                ballCollision.overlap.normal = vec(0, -1);
                g_Globals.ballCollisionMtx.lock();
                g_Globals.ballCollisions.push_back(ballCollision);
                g_Globals.ballCollisionMtx.unlock();
            }

            for (auto [x, y] : ballTileRef.tiles)
            {
                for (auto brickId : g_Globals.tilesBricks[y * tileCountX + x].ids)
                {
                    auto [brickPos, brickSize] = ecs->getComponents<Position, Size>(brickId);
                    BallCollision ballCollision;
                    bool overlap = test_circle_aabb_overlap(ballCollision.overlap, ballPos, Globals::ballRadius, *brickPos - *brickSize * 0.5f, *brickPos + *brickSize * 0.5f, false);
                    if (overlap)
                    {
                        auto [hasBrick, hasPaddle] = ecs->hasEachComponent<Brick, Paddle>(brickId);

                        ballCollision.ballId = ballId;
                        ballCollision.otherObjectId = brickId;
                        if (hasBrick)
                        {
                            it.getView()->addComponent<CollidedWithBall>(brickId);
                        }
                        else if (hasPaddle)
                        {
                            float toBall = ballPos.x - brickPos->x;
                            toBall /= brickSize->x;
                            ballCollision.overlap.normal.x = lerp(0.0f, 0.9f, toBall);
                            ballCollision.overlap.normal = vec_normalize(ballCollision.overlap.normal);
                        }
                        g_Globals.ballCollisionMtx.lock();
                        g_Globals.ballCollisions.push_back(ballCollision);
                        g_Globals.ballCollisionMtx.unlock();
                    }
                }
            }
        };
        JOB_SCHEDULE(updateBallCollisions);
    }
};

struct ResolveBallCollisions : public ecs::System
{
    void scheduleJobs() override
    {
        for (auto& ballCollision : g_Globals.ballCollisions)
        {
            auto [vel] = ecs->getComponents<Velocity>(ballCollision.ballId);
            // this assumes the bricks are not moving!
            // so that their relative velocities are the same as the ball's velocity
            if (vec_dot(*vel, ballCollision.overlap.normal) > 0)
                continue;

            (vec&)*vel = vec_reflect(*vel, ballCollision.overlap.normal);
            int a = 56;
            a = 235;
            //(vec&)*pos += ballCollision.overlap.normal * ballCollision.overlap.penetration;
        }
    }
};

struct HandleBrickCollisions : public ecs::System
{
    void scheduleJobs() override
    {
        auto brickCollisions = ecs::Job(ecs->view<Position, Size, TileReference, Brick>().with<CollidedWithBall>());
        JOB_SET_FN(brickCollisions)
        {
            auto& [brickId, pos, size, tileRef, brick] = *it;
            if (brick.type == Brick::Type::Ballspawner)
            {
                vec ballPosition = pos - vec(0, size.y + Globals::ballRadius);
                vec ballVelocity = vec(0, -Globals::ballStartingSpeed);
                auto newBallId = it.getView()->createEntity(g_Globals.prefabs.spawnedBall, Position{ ballPosition }, Velocity{ ballVelocity });
            }
            it.getView()->deleteEntity(brickId);
        };
        JOB_SCHEDULE(brickCollisions);
    }
};

bool has_balls_in_play()
{
    auto balls = getEcs().view<>().with<Ball>();
    return balls.getCount() > 0;
}

struct UpdateVisibility : public ecs::System
{
    void scheduleJobs() override
    {
        auto [cameraPos, cameraSize] = ecs->getComponents<Position, Size>(g_Globals.camera);
        vec cameraMin = *cameraPos - *cameraSize * 0.5f;
        vec cameraMax = *cameraPos + *cameraSize * 0.5f;

        auto fnIsVisible = [cameraMin, cameraMax](const vec& pos, const vec& size) -> bool
        {
            vec objectMin = pos - size * 0.5f;
            vec objectMax = pos + size * 0.5f;

            bool outside =
                objectMin.x > cameraMax.x ||
                objectMin.y > cameraMax.y ||
                objectMax.x < cameraMin.x ||
                objectMax.y < cameraMin.y;

            return !outside;
        };

        auto hide = ecs::Job(ecs->view<const Position, const Size>().filterShared(Visibility{ true }));
        JOB_SET_FN(hide)
        {
            auto& [id, pos, size] = *it;
            if (!fnIsVisible(pos, size))
                it.getView()->setSharedComponentData(id, Visibility{ false });
        };
        JOB_SCHEDULE(hide);

        auto show = ecs::Job(ecs->view<const Position, const Size>().filterShared(Visibility{ false }));
        JOB_SET_FN(show)
        {
            auto& [id, pos, size] = *it;
            if (fnIsVisible(pos, size))
                it.getView()->setSharedComponentData(id, Visibility{ true });
        };
        JOB_SCHEDULE(show);
    }
};

void render_sprites()
{
    sf::Sprite sprite;
    Sprite usedSpriteDesc;
    vec sizeCorrection = { 1.0f, 1.0f };

    auto view = getEcs().view<Position, Size>().with<Sprite>().filterShared(Visibility{ true });
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        auto& [id, pos, size] = *it;
        auto& currentSpriteDesc = *it.getSharedComponent<Sprite>();

        if (!ecs::equals(usedSpriteDesc, currentSpriteDesc))
        {
            sprite = sf::Sprite();
            usedSpriteDesc = currentSpriteDesc;
            sprite.setTexture(g_Globals.textures[usedSpriteDesc.index]);
            auto textureSize = sprite.getTexture()->getSize();
            sprite.setOrigin(textureSize.x * 0.5f, textureSize.y * 0.5f);
            sizeCorrection = vec(1.0f / textureSize.x, 1.0f / textureSize.y);
            sprite.setColor(usedSpriteDesc.color);
        }
        vec scale = vec(size.x * sizeCorrection.x, size.y * sizeCorrection.y);
        vec center = pos;
        center.y = g_Globals.screenSize.y - center.y;
        sprite.setPosition(center);
        sprite.setScale(scale);
        g_Globals.window.draw(sprite);
    }
}

void render_tile_debug()
{
    ecs::entityId ballId = 0;
    auto& ecs = getEcs();
    for (auto& [id, ball] : getEcs().view<Ball>())
    {
        ballId = id;
        break;
    }

    int tileCountX = get_tile_count_x();
    int tileCountY = get_tile_count_y();
    for (int y = 0; y < tileCountY; y++)
    {
        for (int x = 0; x < tileCountX; x++)
        {
            auto& tileBall = g_Globals.tilesBalls[y * tileCountX + x];
            auto it = std::find(tileBall.ids.begin(), tileBall.ids.end(), ballId);
            bool includesBall = it != tileBall.ids.end();
            vec tileCenter(x * Globals::tileSize + Globals::tileSize * 0.5f, y * Globals::tileSize + Globals::tileSize * 0.5f);
            render_rect(tileCenter, vec(Globals::tileSize, Globals::tileSize) * 0.9f, includesBall ? sf::Color(0, 255, 0, 120) : sf::Color(255, 0, 0, 60));
        }
    }
}

struct UpdateBallMagnet : public ecs::System
{
    void scheduleJobs() override
    {
        if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            return;
        }

        auto updateBallMagnet = ecs::Job(ecs->view<const Position, Velocity>().with<Ball>());
        JOB_SET_FN(updateBallMagnet)
        {
            auto& [ballId, ballPos, ballVelocity] = *it;
            for (auto& [paddleId, paddlePos] : ecs->view<const Position>().with<Paddle>())
            {
                vec toPaddle = paddlePos - ballPos;
                vec toPaddleDir = vec_normalize(toPaddle);
                vec ballDir = vec_normalize(ballVelocity);

                float dot = vec_dot(ballDir, toPaddleDir);
                bool ccw = vec_ccw(ballDir, toPaddleDir);

                float angle = acosf(clamp(dot, -1.0f, 1.0f));
                if (!ccw)
                    angle *= -1;

                static float turnRate = 2.0f;
                float maxAngle = g_Globals.elapsedTime * turnRate;
                angle = clamp(angle, -maxAngle, maxAngle);

                ballDir = vec_rotate(ballDir, angle);

                (vec&)ballVelocity = ballDir * Globals::ballStartingSpeed;
            }
        };

        JOB_SCHEDULE(updateBallMagnet);
    }
};

void fix_up_horizontal_ball_velocities()
{
    EASY_FUNCTION();
    for (auto& [ballId, ballVelocity] : getEcs().view<Velocity>().with<Ball>())
    {
        if (fabsf(ballVelocity.y) < 15)
        {
            ballVelocity.y = -15;
            (vec&)ballVelocity = vec_normalize(ballVelocity) * Globals::ballStartingSpeed;
            int alma = 523;
            alma = 23;
        }
    }
}

void update_ball_respawns()
{
    EASY_FUNCTION();
    if (!has_balls_in_play())
    {
        if (g_Globals.ballRespawnTimer < 0)
        {
            g_Globals.ballRespawnTimer = 1.0f;
        }
    }

    if (g_Globals.ballRespawnTimer >= 0.0f)
    {
        g_Globals.ballRespawnTimer -= g_Globals.elapsedTime;
        if (g_Globals.ballRespawnTimer < 0)
        {
            spawn_ball_on_paddle();
        }
    }
}

void update_camera_move()
{
    EASY_FUNCTION();
    vec cameraMove;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) cameraMove.x -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) cameraMove.x += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) cameraMove.y += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) cameraMove.y -= 1.0f;

    if (!vec_is_zero(cameraMove))
    {
        static float cameraMoveSpeed = 300.0f;
        Position* cameraPos = getEcs().getComponent<Position>(g_Globals.camera);
        (vec&)*cameraPos += cameraMove * (cameraMoveSpeed * g_Globals.elapsedTime);
    }
}

struct UpdateBallTrailParticles : public ecs::System
{
    void scheduleJobs() override
    {
        float elapsedTime = g_Globals.elapsedTime;
        float emitterInterval = 0.01f;

        auto emitJob = ecs::Job(ecs->view<const Position, ParticleEmitter>().exclude<AttachedToPaddle>());
        JOB_SET_FN(emitJob)
        {
            auto& [id, pos, emitter] = *it;

            emitter.timeUntilNextEmit -= elapsedTime;
            if (emitter.timeUntilNextEmit < 0.0f)
            {
                emitter.timeUntilNextEmit = emitterInterval;
                it.getView()->createEntity(g_Globals.prefabs.particle, pos);
            }
        };
        JOB_SCHEDULE(emitJob);

        auto updateParticlesJob = ecs::Job(ecs->view<Particle>());
        JOB_SET_FN(updateParticlesJob)
        {
            auto& [id, particle] = *it;
            particle.timeToLive -= elapsedTime;
            if (particle.timeToLive < 0.0f)
                it.getView()->deleteEntity(id);
        };
        JOB_SCHEDULE(updateParticlesJob);
    }
};

void update()
{
    EASY_FUNCTION();
    sf::Clock clock;
    auto& scheduler = getScheduler();

    scheduler.scheduleSystem<CreateTileReferencesForNewEntities>();
    scheduler.runSystems();

    update_paddle_by_mouse();
    update_balls_attached_to_paddle();
    
    scheduler.scheduleSystem<UpdateBallMagnet>();
    scheduler.runSystems();

    fix_up_horizontal_ball_velocities();

    scheduler.scheduleSystem<TiledPositionByVelocitySystem>();
    scheduler.scheduleSystem<SimplePositionByVelocitySystem>();
    scheduler.runSystems();

    scheduler.scheduleSystem<UpdateBallCollisionsTiled>();
    scheduler.scheduleSystem<UpdateBallTrailParticles>();
    scheduler.runSystems();

    scheduler.scheduleSystem<ResolveBallCollisions>();
    scheduler.scheduleSystem<HandleBrickCollisions>();
    scheduler.runSystems();

    update_ball_respawns();
    update_camera_move();

    clear_tile_references_for_deleted_entities();

    if (getEcs().view<>().with<ecs::DeletedEntity>().getCount())
    {
        printf("Deleted entites are still in this fucking game!");
    }

    sf::Time elapsedTime = clock.restart();
    g_Globals.gamelogicUpdateTime = elapsedTime.asSeconds();
}

void setViewFromCamera()
{
    auto& ecs = getEcs();
    auto [cameraPos, cameraSize] = ecs.getComponents<const Position, const Size>(g_Globals.camera);
    vec pos = *cameraPos;
    vec size = *cameraSize;
    pos.y = g_Globals.screenSize.y - pos.y;
    g_Globals.window.setView(sf::View(sf::FloatRect(pos - size * 0.5f, size)));
}

sf::Text text;
sf::Font font;

void render_stats()
{
    g_Globals.window.setView(sf::View(sf::FloatRect(vec(0.0f, 0.0f), g_Globals.screenSize)));

    EASY_FUNCTION();
    char debugString[1024];
    sprintf_s(debugString, "Frame time: %0.2f ms", g_Globals.elapsedTime * 1000.0f);
    text.setFont(font);
    text.setPosition(10, 10);
    text.setFillColor(sf::Color::Green);
    text.setString(debugString);
    g_Globals.window.draw(text);

    sprintf_s(debugString, "Update time: %0.2f ms", g_Globals.gamelogicUpdateTime * 1000.0f);
    text.setString(debugString);
    text.setPosition(10, 40);
    g_Globals.window.draw(text);
}

void render()
{
    EASY_FUNCTION();
    setViewFromCamera();

    getScheduler().scheduleSystem<UpdateVisibility>();
    getScheduler().runSystems();

    render_sprites();
    //render_tile_debug();
    render_stats();
}

void save_ecs()
{
    std::ofstream stream("saved_scene.brk", std::ios::out | std::ios::binary);
    g_Globals.ecs.save(stream);
    stream.write((const char*)&g_Globals.camera, sizeof(g_Globals.camera));
    stream.close();
}

void load_ecs()
{
    std::ifstream stream;
    stream.open("saved_scene.brk", std::ios::binary);
    g_Globals.ecs.load(stream);
    stream.read((char*)&g_Globals.camera, sizeof(g_Globals.camera));
    stream.close();

    g_Globals.ballRespawnTimer = -1.0f;
    g_Globals.ballCollisions.clear();
    init_tiles();
}

int main()
{
    EASY_PROFILER_ENABLE;
    profiler::startListen();

    EASY_MAIN_THREAD;

    init_globals();
    setup_level(g_Globals.gameState);

    font.loadFromFile("arial.ttf");

    auto& window = g_Globals.window;
    window.setMouseCursorVisible(false);
    window.setMouseCursorGrabbed(true);

    spawn_ball_on_paddle();

    sf::Clock clock;

    // run the program as long as the window is open
    while (window.isOpen())
    {
        sf::Time elapsedTime = clock.restart();
        g_Globals.elapsedTime = elapsedTime.asSeconds();
        g_Globals.elapsedTime *= g_Globals.timeMultipler;
        g_Globals.elapsedTime = std::min(g_Globals.elapsedTime, 1.0f / 60);

        if (g_Globals.isPaused)
            g_Globals.elapsedTime = 0.0f;

        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::KeyPressed)
            {
                switch (event.key.code)
                {
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                case sf::Keyboard::D:
                    g_debugBall = !g_debugBall;
                    break;
                case sf::Keyboard::P:
                    g_Globals.isPaused = !g_Globals.isPaused;
                    break;
                case sf::Keyboard::Subtract:
                    g_Globals.timeMultipler = std::max(0.0f, g_Globals.timeMultipler - 0.1f);
                    break;
                case sf::Keyboard::Add:
                    g_Globals.timeMultipler = std::max(0.0f, g_Globals.timeMultipler + 0.1f);
                    break;
                case sf::Keyboard::M:
                    getScheduler().singleThreadedMode = !getScheduler().singleThreadedMode;
                    break;
                case sf::Keyboard::F5:
                    save_ecs();
                    break;
                case sf::Keyboard::F6:
                    load_ecs();
                    break;
                }
            }
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    fire_ball();
                }
            }
        }

        // clear the window with black color
        window.clear(sf::Color::Blue);

        update();
        render();

        {
            EASY_BLOCK("Display");
            // end the current frame
            window.display();
        }
    }

    return 0;
}

//void TileReference::save(std::ostream& stream) const
//{
//    stream.write((const char*)&isBall, sizeof(isBall));
//    ecs::saveVector(stream, tiles);
//}
//
//void TileReference::load(std::istream& stream)
//{
//    stream.read((char*)&isBall, sizeof(isBall));
//    ecs::loadVector(stream, tiles);
//}
