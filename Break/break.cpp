#include "break.h"

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

    g_Globals.tilesBalls.clear();
    g_Globals.tilesBalls.resize(tileCountX * tileCountY);

    g_Globals.tilesBricks.clear();
    g_Globals.tilesBricks.resize(tileCountX * tileCountY);
}

void init_globals()
{
    //g_Globals.gameState  paddle.position.y = Globals::paddleHeight * 0.5f;
    g_Globals.rectPrototype.setSize(vec{ 1.0f, 1.0f });
    g_Globals.rectPrototype.setOrigin(vec{0.5f, 0.5f});
    g_Globals.rectPrototype.setFillColor(sf::Color::White);

    g_Globals.circlePrototype.setRadius(1.0f);
    g_Globals.circlePrototype.setFillColor(sf::Color(100, 100, 100));

    auto& ecs = getEcs();
    ecs.registerType<Position>("Position");
    ecs.registerType<Size>("Size");
    ecs.registerType<Velocity>("Velocity");
    ecs.registerType<TileReferenceCreator>("TileReferenceCreator");
    ecs.registerType<TileReference>("TileReference");
    ecs.registerType<Paddle>("Paddle");
    ecs.registerType<AttachedToPaddle>("AttachedToPaddle");
    ecs.registerType<Ball>("Ball");
    ecs.registerType<Brick>("Brick");
    ecs.registerType<CollidedWithBall>("CollidedWithBall");
    ecs.registerType<Camera>("Camera");
    ecs.registerType<Visible>("Visible");

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

    auto& tiles = tileRef.isBall ? g_Globals.tilesBalls : g_Globals.tilesBricks;

    for (int y = firstTileY; y <= lastTileY; y++)
    {
        for (int x = firstTileX; x <= lastTileX; x++)
        {
            tiles[y * tileCountX + x].ids.push_back(id);
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
                auto& ids = tiles[y * tileCountX + x].ids;
                auto it = std::find(ids.begin(), ids.end(), id);
                if (it != ids.end())
                {
                    unordered_delete(ids, it);
                    auto itTileRef = std::find(tileRef.tiles.begin(), tileRef.tiles.end(), std::make_pair(x, y));
                    if (itTileRef != tileRef.tiles.end())
                        unordered_delete(tileRef.tiles, itTileRef);
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
                auto& ids = tiles[y * tileCountX + x].ids;
                auto it = std::find(ids.begin(), ids.end(), id);
                if (it == ids.end())
                {
                    ids.push_back(id);
                    tileRef.tiles.push_back(std::make_pair(x, y));
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
        auto& ids = tiles[y * tileCountX + x].ids;
        auto it = std::find(ids.begin(), ids.end(), id);
        if (it != ids.end())
            unordered_delete(ids, it);
    }
}

void create_tile_references_for_new_entities()
{
    EASY_FUNCTION();
    auto& ecs = getEcs();
    auto v = ecs::View<Position, Size, TileReferenceCreator>(ecs).with<>();
    for (auto& [it, id, pos, size, tileReferenceCreator] : v)
    {
        TileReference tileRef{ tileReferenceCreator.isBall };
        insert_into_tiles(id, pos, size, tileRef);
        v.addComponent<TileReference>(id, tileRef);
        v.deleteComponents<TileReferenceCreator>(id);
    }
}

void setup_level(GameState& gamestate)
{
    auto& ecs = getEcs();

    // Create the paddle
    auto pos = Position{ g_Globals.screenSize.x * 0.5f, Globals::paddleHeight * 0.5f };
    auto size = Size{ 150.0f, Globals::paddleHeight };
    entityId paddleId = ecs.createEntity(g_Globals.prefabs.paddle, pos, size);

    int rows = 25;
    int columns = 25;

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
            if (rand() % 10 == 0)
                brickType = Brick::Type::Ballspawner;

            ecs::entityId id = ecs.createEntity(g_Globals.prefabs.brick, Position{ currentBrickPos }, Size{ brickSize }, Brick{ brickType });
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
    ecs::View<Position, Size, Paddle> paddleView(ecs);
    auto itPaddle = paddleView.begin();
    if (itPaddle == paddleView.end())
        return;

    auto& [it, paddleId, pos, size, paddle] = *itPaddle;
    vec ballPosition = pos + vec(0.0f, size.y * 0.5f + Globals::ballRadius);

    ecs::entityId ballId = ecs.createEntity(g_Globals.prefabs.attachedBall,
        Position{ ballPosition },
        AttachedToPaddle{ paddleId, vec(0.0f, size.y * 0.5f + Globals::ballRadius)
    });
}

void fire_ball()
{
    auto view = ecs::View<Velocity>(getEcs()).with<Ball, AttachedToPaddle>();
    for (auto& [it, id, vel] : view)
    {
        (vec&)vel = vec_normalize(vec{ 1,1 }) * Globals::ballStartingSpeed;
        view.deleteComponents<AttachedToPaddle>(id);
    }
}

void update_positions_by_velocities()
{
    EASY_FUNCTION();
    float dt = g_Globals.elapsedTime;
    for (auto& [it, id, pos, vel, tileRef] : ecs::View<Position, Velocity, TileReference>(getEcs()))
    {
        if (vel.x == 0.0f && vel.y == 0.0)
            continue;

        auto oldPos = pos;
        pos += vel * dt;
        update_tiles_after_move(id, oldPos, pos, Size(Globals::ballRadius, Globals::ballRadius), tileRef);
    }
}

void update_paddle_by_mouse()
{
    EASY_FUNCTION();
    vec mousePosition{ sf::Mouse::getPosition(g_Globals.window) };
    float screenSizeX = g_Globals.screenSize.x;
    for (auto& [it, id, pos, size, tileRef] : ecs::View<Position, Size, TileReference>(getEcs()).with<Paddle>())
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
    for (auto& [it, id, pos, tileRef, attach] : ecs::View<Position, TileReference, const AttachedToPaddle>(ecs))
    {
        auto paddlePos = ecs.getComponent<Position>(attach.paddleId);
        if (!paddlePos)
            continue;
        auto oldPos = pos;
        pos = *paddlePos + attach.relativePos;
        update_tiles_after_move(id, oldPos, pos, Size(Globals::ballRadius, Globals::ballRadius), tileRef);
    }
}

void update_ball_collisions_tiled()
{
    EASY_FUNCTION();
    g_Globals.ballCollisions.clear();
    auto& ecs = getEcs();

    int tileCountX = get_tile_count_x();
    int tileCountY = get_tile_count_y();
    auto ballView = ecs::View<Position, TileReference>(ecs).with<Ball>();
    for (auto& [it, ballId, ballPos, ballTileRef] : ballView)
    {
        bool leftEdge = false, rightEdge = false, topEdge = false, bottomEdge = false;
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

        if(leftEdge && ballPos.x - Globals::ballRadius < 0)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = -(ballPos.x - Globals::ballRadius);
            ballCollision.overlap.collisionPoint = vec(0, ballPos.y);
            ballCollision.overlap.normal = vec(1, 0);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        if(rightEdge && ballPos.x + Globals::ballRadius > g_Globals.screenSize.x)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = ballPos.x + Globals::ballRadius - g_Globals.screenSize.x;
            ballCollision.overlap.collisionPoint = vec(g_Globals.screenSize.x, ballPos.y);
            ballCollision.overlap.normal = vec(-1, 0);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        if(topEdge && ballPos.y + Globals::ballRadius > g_Globals.screenSize.y)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = ballPos.y + Globals::ballRadius - g_Globals.screenSize.y;
            ballCollision.overlap.collisionPoint = vec(ballPos.x, g_Globals.screenSize.y);
            ballCollision.overlap.normal = vec(0, -1);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        if(bottomEdge && ballPos.y < 0)
        {   // Ball dies
            update_tiles_for_deletion(ballId, ballTileRef);
            ballView.deleteEntity(ballId);
            break;
        }

        for (auto [x, y] : ballTileRef.tiles)
        {
            for (auto brickId : g_Globals.tilesBricks[y * tileCountX + x].ids)
            {
                auto [brickPos, brickSize, paddle, brick] = ecs.getComponents<Position, Size, Paddle, Brick>(brickId);
                BallCollision ballCollision;
                bool overlap = test_circle_aabb_overlap(ballCollision.overlap, ballPos, Globals::ballRadius, *brickPos - *brickSize * 0.5f, *brickPos + *brickSize * 0.5f, false);
                if (overlap)
                {
                    ballCollision.ballId = ballId;
                    ballCollision.otherObjectId = brickId;
                    if (brick)
                    {
                        ballView.addComponent<CollidedWithBall>(brickId);
                    }
                    else if (paddle)
                    {
                        float toBall = ballPos.x - brickPos->x;
                        toBall /= brickSize->x;
                        ballCollision.overlap.normal.x = lerp(0.0f, 0.9f, toBall);
                        ballCollision.overlap.normal = vec_normalize(ballCollision.overlap.normal);
                    }
                    g_Globals.ballCollisions.push_back(ballCollision);
                }
            }
        }
    }
}

void resolve_ball_collisions()
{
    EASY_FUNCTION();
    auto& ecs = getEcs();
    for (auto& ballCollision : g_Globals.ballCollisions)
    {
        auto [vel] = ecs.getComponents<Velocity>(ballCollision.ballId);
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

void handle_brick_collisions()
{
    EASY_FUNCTION();
    auto& ecs = getEcs();
    auto brickView = ecs::View<Position, Size, TileReference, Brick>(ecs);
    for (auto& [it, brickId, pos, size, tileRef, brick] : brickView.with<CollidedWithBall>())
    {
        if (brick.type == Brick::Type::Ballspawner)
        {
            vec ballPosition = pos - vec(0, size.y + Globals::ballRadius);
            vec ballVelocity = vec(0, -Globals::ballStartingSpeed);
            auto newBallId = brickView.createEntity(g_Globals.prefabs.spawnedBall, Position{ ballPosition }, Velocity{ ballVelocity });
        }
        update_tiles_for_deletion(brickId, tileRef);
        brickView.deleteEntity(brickId);
    }
}

bool has_balls_in_play()
{
    auto balls = ecs::View<>(getEcs()).with<Ball>();
    return balls.getCount() > 0;
}

void update_visibility()
{
    EASY_FUNCTION();
    auto [cameraPos, cameraSize] = getEcs().getComponents<Position, Size>(g_Globals.camera);
    vec cameraMin = *cameraPos - *cameraSize * 0.5f;
    vec cameraMax = *cameraPos + *cameraSize * 0.5f;

    auto fnIsVisible = [cameraMin, cameraMax](const vec& objectMin, const vec& objectMax) -> bool
    {
        bool outside = 
            objectMin.x > cameraMax.x ||
            objectMin.y > cameraMax.y ||
            objectMax.x < cameraMin.x ||
            objectMax.y < cameraMin.y;

        return !outside;
    };

    auto v = ecs::View<const Position, const Size>(getEcs());
    for (auto& [it, id, pos, size] : v)
    {
        vec objectMin = pos - size * 0.5f;
        vec objectMax = pos + size * 0.5f;

        if (it.hasComponents<Visible>())
        {
            if(!fnIsVisible(objectMin, objectMax))
                v.deleteComponents<Visible>(id);
        }
        else
        {
            if (fnIsVisible(objectMin, objectMax))
            {
                v.addComponent<Visible>(id, Visible{});
            }
        }
    }

}

void render_paddle()
{
    for (auto& [it, id, pos, size, paddle] : ecs::View<Position, Size, Paddle>(getEcs()).with<Visible>())
    {
        render_rect(pos, size, sf::Color(50, 50, 200)); 
    }

}

void render_balls()
{
    for (auto& [it, id, pos, ball] : ecs::View<Position, Ball>(getEcs()).with<Visible>())
    {
        render_circle(pos, Globals::ballRadius, sf::Color(100, 100, 100));
    }
}

void render_bricks()
{
    for (auto& [it, id, pos, size, brick] : ecs::View<Position, Size, Brick>(getEcs()).with<Visible>())
    {
        render_rect(pos, size, brick.type == Brick::Type::Simple ? sf::Color::Magenta : sf::Color::Yellow);
    }
}

void render_tile_debug()
{
    ecs::entityId ballId = 0;
    auto& ecs = getEcs();
    for (auto& [it, id, ball] : ecs::View<Ball>(ecs))
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

void update_ball_magnet()
{
    EASY_FUNCTION();
    if (!sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
    {
        return;
    }

    auto& ecs = getEcs();
    for (auto& [it, paddleId, paddlePos] : ecs::View<const Position>(ecs).with<Paddle>())
    {
        for (auto& [it, ballId, ballPos, ballVelocity] : ecs::View<const Position, Velocity>(ecs).with<Ball>())
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
    }
}

void fix_up_horizontal_ball_velocities()
{
    EASY_FUNCTION();
    for (auto& [it, ballId, ballVelocity] : ecs::View<Velocity>(getEcs()).with<Ball>())
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
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) cameraMove.y -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) cameraMove.y += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) cameraMove.x += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) cameraMove.x -= 1.0f;

    if (!vec_is_zero(cameraMove))
    {
        static float cameraMoveSpeed = 300.0f;
        Position* cameraPos = getEcs().getComponent<Position>(g_Globals.camera);
        (vec&)*cameraPos += cameraMove * (cameraMoveSpeed * g_Globals.elapsedTime);
    }
}

void update()
{
    EASY_FUNCTION();
    create_tile_references_for_new_entities();
    update_paddle_by_mouse();
    update_balls_attached_to_paddle();
    update_ball_magnet();
    fix_up_horizontal_ball_velocities();

    update_positions_by_velocities();
    update_ball_collisions_tiled();
    resolve_ball_collisions();
    handle_brick_collisions();

    update_ball_respawns();
    update_camera_move();
}

void setViewFromCamera()
{
    auto& ecs = getEcs();
    auto [cameraPos, cameraSize] = ecs.getComponents<Position, Size>(g_Globals.camera);
    g_Globals.window.setView(sf::View(sf::FloatRect(*cameraPos - (*cameraSize) * 0.5f, *cameraSize)));
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
}

void render()
{
    EASY_FUNCTION();
    setViewFromCamera();
    update_visibility();
    render_paddle();
    render_balls();
    render_bricks();
    //render_tile_debug();
    render_stats();
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
        g_Globals.elapsedTime = std::min(g_Globals.elapsedTime, 1.0f / 60);
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
