#include "break.h"

const float paddleHeight = 25.0f;
const float ballRadius = 20.0f;
const float ballStartingSpeed = 400.0f;
const float tileSize = 50.0f;

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
    int tileCountX = (int)(g_Globals.screenSize.x / tileSize);
    return tileCountX;
}

int get_tile_count_y()
{
    int tileCountY = (int)(g_Globals.screenSize.y / tileSize);
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
    //g_Globals.gameState  paddle.position.y = paddleHeight * 0.5f;
    g_Globals.rectPrototype.setSize(vec{ 1.0f, 1.0f });
    g_Globals.rectPrototype.setOrigin(vec{0.5f, 0.5f});
    g_Globals.rectPrototype.setFillColor(sf::Color::White);

    g_Globals.circlePrototype.setRadius(1.0f);
    g_Globals.circlePrototype.setFillColor(sf::Color(100, 100, 100));

    auto& ecs = getEcs();
    ecs.registerType<Position>("Position");
    ecs.registerType<Size>("Size");
    ecs.registerType<Velocity>("Velocity");
    ecs.registerType<TileReference>("TileReference");
    ecs.registerType<Paddle>("Paddle");
    ecs.registerType<AttachedToPaddle>("AttachedToPaddle");
    ecs.registerType<Ball>("Ball");
    ecs.registerType<Brick>("Brick");      

    init_tiles();
}

void insert_into_tiles(ecs::entityId id, const Position& pos, const Size& size, TileReference& tileRef)
{
    vec min = pos - size * 0.5f;
    vec max = pos + size * 0.5f;

    int firstTileX = (int)(min.x / tileSize);
    int lastTileX = (int)(max.x / tileSize);

    int firstTileY = (int)(min.y / tileSize);
    int lastTileY = (int)(max.y / tileSize);

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

    int oldFirstTileX = (int)(oldMin.x / tileSize);
    int oldLastTileX = (int)(oldMax.x / tileSize);

    int oldFirstTileY = (int)(oldMin.y / tileSize);
    int oldLastTileY = (int)(oldMax.y / tileSize);

    vec newMin = newPos - size * 0.5f;
    vec newMax = newPos + size * 0.5f;

    int newFirstTileX = (int)(newMin.x / tileSize);
    int newLastTileX = (int)(newMax.x / tileSize);

    int newFirstTileY = (int)(newMin.y / tileSize);
    int newLastTileY = (int)(newMax.y / tileSize);

    int tileCountX = get_tile_count_x();

    auto& tiles = tileRef.isBall ? g_Globals.tilesBalls : g_Globals.tilesBricks;

    for (int y = oldFirstTileY; y <= oldLastTileY; y++)
    {
        bool isOutsideY = (y < newFirstTileY || y > newLastTileY);
        for (int x = oldFirstTileX; x <= oldLastTileX; x++)
        {
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
        bool isOutsideY = (y < oldFirstTileY || y > oldLastTileY);
        for (int x = newFirstTileX; x <= newLastTileX; x++)
        {
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

void setup_level(GameState& gamestate)
{
    auto& ecs = getEcs();

    // Create the paddle
    auto pos = Position{ g_Globals.screenSize.x * 0.5f, paddleHeight * 0.5f };
    auto size = Size{ 150.0f, paddleHeight };
    entityId paddleId = ecs.createEntity<Position, Size, TileReference, Paddle>(pos, size, TileReference{false}, Paddle{});
    auto tileRef = ecs.getComponent<TileReference>(paddleId);
    insert_into_tiles(paddleId, pos, size, *tileRef);

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
            //if (rand() % 10 == 0)
            //    brickType = Brick::Type::Ballspawner;

            ecs::entityId id = ecs.createEntity<Position, Size, TileReference, Brick>(Position{ currentBrickPos }, Size{ brickSize }, TileReference{ false }, Brick{ brickType });
            auto tileRef = ecs.getComponent<TileReference>(id);
            insert_into_tiles(id, Position{ currentBrickPos }, Size{ brickSize }, *tileRef);

            currentBrickPos.x += brickSize.x + brickSpacing;
        }
        currentBrickPos.x = originalBrickPosition.x;
        currentBrickPos.y -= brickSize.y + brickSpacing;
    }
}

void spawn_ball_on_paddle()
{
    auto& ecs = getEcs();
    ecs::View<Position, Size, Paddle> paddleView(ecs);
    auto itPaddle = paddleView.begin();
    if (itPaddle == paddleView.end())
        return;

    auto& [paddleId, pos, size, paddle] = *itPaddle;
    vec ballPosition = pos + vec(0.0f, size.y * 0.5f + ballRadius);

    ecs::entityId ballId = ecs.createEntity<Position, Velocity, Ball, TileReference, AttachedToPaddle>(
        Position{ ballPosition }, 
        Velocity{}, 
        Ball{}, 
        TileReference{ true },
        AttachedToPaddle{ paddleId, vec(0.0f, size.y * 0.5f + ballRadius)
    });

    auto tileRef = ecs.getComponent<TileReference>(ballId);

    insert_into_tiles(ballId, Position{ ballPosition }, Size{ ballRadius * 2, ballRadius * 2}, *tileRef);
}

void fire_ball()
{
    ecs::View<Ball, AttachedToPaddle, Velocity> view(getEcs());
    for (auto& [id, ball, attach, vel] : view)
    {
        (vec&)vel = vec_normalize(vec{ 1,1 }) * ballStartingSpeed;
        view.deleteComponents<AttachedToPaddle>(id);
    }
}

void update_positions_by_velocities()
{
    float dt = g_Globals.elapsedTime;
    for (auto& [id, pos, vel, tileRef] : ecs::View<Position, Velocity, TileReference>(getEcs()))
    {
        if (vel.x == 0.0f && vel.y == 0.0)
            continue;

        auto oldPos = pos;
        pos += vel * dt;
        update_tiles_after_move(id, oldPos, pos, Size(ballRadius, ballRadius), tileRef);
    }
}

void update_paddle_by_mouse()
{
    vec mousePosition{ sf::Mouse::getPosition(g_Globals.window) };
    float screenSizeX = g_Globals.screenSize.x;
    for (auto& [id, pos, size, tileRef, paddle] : ecs::View<Position, Size, TileReference, Paddle>(getEcs()))
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
    auto& ecs = getEcs();
    for (auto& [id, pos, tileRef, attach] : ecs::View<const Position, TileReference, AttachedToPaddle>(ecs))
    {
        auto paddlePos = ecs.getComponent<Position>(attach.paddleId);
        if (!paddlePos)
            continue;
        auto oldPos = pos;
        (vec&)pos = *paddlePos + attach.relativePos;
        update_tiles_after_move(id, oldPos, pos, Size(ballRadius, ballRadius), tileRef);
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
    for (auto& [ballId, ballPos, ballTileRef] : ballView)
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

        if(leftEdge && ballPos.x - ballRadius < 0)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = -(ballPos.x - ballRadius);
            ballCollision.overlap.collisionPoint = vec(0, ballPos.y);
            ballCollision.overlap.normal = vec(1, 0);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        if(rightEdge && ballPos.x + ballRadius > g_Globals.screenSize.x)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = ballPos.x + ballRadius - g_Globals.screenSize.x;
            ballCollision.overlap.collisionPoint = vec(g_Globals.screenSize.x, ballPos.y);
            ballCollision.overlap.normal = vec(-1, 0);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        if(topEdge && ballPos.y + ballRadius > g_Globals.screenSize.y)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = ballPos.y + ballRadius - g_Globals.screenSize.y;
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
                bool overlap = test_circle_aabb_overlap(ballCollision.overlap, ballPos, ballRadius, *brickPos - *brickSize * 0.5f, *brickPos + *brickSize * 0.5f, false);
                if (overlap)
                {
                    ballCollision.ballId = ballId;
                    ballCollision.otherObjectId = brickId;
                    if (brick)
                    {
                        brick->wasHitThisFrame = true;
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

void update_ball_collisions()
{
    EASY_FUNCTION();
    g_Globals.ballCollisions.clear();
    auto pos_ball = ecs::View<Position>(getEcs()).with<Ball>();
    for (auto& [ballId, ballPos] : pos_ball)
    {
        // Collide with the screen edge
        if (ballPos.x - ballRadius < 0)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = -(ballPos.x - ballRadius);
            ballCollision.overlap.collisionPoint = vec(0, ballPos.y);
            ballCollision.overlap.normal = vec(1, 0);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        else if (ballPos.x + ballRadius > g_Globals.screenSize.x)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = ballPos.x + ballRadius - g_Globals.screenSize.x;
            ballCollision.overlap.collisionPoint = vec(g_Globals.screenSize.x, ballPos.y);
            ballCollision.overlap.normal = vec(-1, 0);
            g_Globals.ballCollisions.push_back(ballCollision);
        }

        if (ballPos.y + ballRadius > g_Globals.screenSize.y)
        {
            BallCollision ballCollision;
            ballCollision.ballId = ballId;
            ballCollision.otherObjectId = 0;
            ballCollision.overlap.penetration = ballPos.y + ballRadius - g_Globals.screenSize.y;
            ballCollision.overlap.collisionPoint = vec(ballPos.x, g_Globals.screenSize.y);
            ballCollision.overlap.normal = vec(0, -1);
            g_Globals.ballCollisions.push_back(ballCollision);
        }
        else if (ballPos.y < 0)
        {   // Ball dies
            pos_ball.deleteEntity(ballId);
            break;
        }

        for (auto& [brickId, brickPos, brickSize, brick] : ecs::View<Position, Size, Brick>(getEcs()))
        {
            BallCollision ballCollision;
            bool overlap = test_circle_aabb_overlap(ballCollision.overlap, ballPos, ballRadius, brickPos - brickSize * 0.5f, brickPos + brickSize * 0.5f, false);
            if (overlap)
            {
                ballCollision.ballId = ballId;
                ballCollision.otherObjectId = brickId;
                brick.wasHitThisFrame = true;
                g_Globals.ballCollisions.push_back(ballCollision);
            }
        }

        for (auto& [paddleId, paddlePos, paddleSize] : ecs::View<Position, Size>(getEcs()).with<Paddle>())
        {
            BallCollision ballCollision;
            bool overlap = test_circle_aabb_overlap(ballCollision.overlap, ballPos, ballRadius, paddlePos - paddleSize * 0.5f, paddlePos + paddleSize * 0.5f, false);
            if (overlap)
            {
                ballCollision.ballId = ballId;
                ballCollision.otherObjectId = paddleId;
                float toBall = ballPos.x - paddlePos.x;
                toBall /= paddleSize.x;
                ballCollision.overlap.normal.x = lerp(0.0f, 0.9f, toBall);
                ballCollision.overlap.normal = vec_normalize(ballCollision.overlap.normal);

                g_Globals.ballCollisions.push_back(ballCollision);
            }
        }
    }
}

void resolve_ball_collisions()
{
    auto& ecs = getEcs();
    for (auto& ballCollision : g_Globals.ballCollisions)
    {
        auto [vel] = ecs.getComponents<Velocity>(ballCollision.ballId);
        // this assumes the bricks are not moving!
        // so that their relative velocities are the same as the ball's velocity
        if (vec_dot(*vel, ballCollision.overlap.normal) > 0)
            continue;

        (vec&)*vel = vec_reflect(*vel, ballCollision.overlap.normal);
        //(vec&)*pos += ballCollision.overlap.normal * ballCollision.overlap.penetration;
    }
}

void handle_brick_collisions()
{
    auto& ecs = getEcs();
    auto brickView = ecs::View<Position, Size, TileReference, Brick>(ecs);
    for (auto& [brickId, pos, size, tileRef, brick] : brickView)
    {
        if (brick.wasHitThisFrame)
        {
            if (brick.type == Brick::Type::Ballspawner)
            {
                vec ballPosition = pos - vec(0, size.y + ballRadius);
                vec ballVelocity = vec(0, -ballStartingSpeed);
                auto newBallId = brickView.createEntity<Position, Velocity, TileReference, Ball>(Position{ ballPosition }, Velocity{ ballVelocity }, TileReference{ true }, Ball{});
                auto tileRef = ecs.getComponent<TileReference>(newBallId);
                insert_into_tiles(newBallId, Position{ ballPosition }, Size{ ballRadius, ballRadius }, *tileRef);
            }
            brick.wasHitThisFrame = false;
            update_tiles_for_deletion(brickId, tileRef);
            brickView.deleteEntity(brickId);
        }
    }
}

bool has_balls_in_play()
{
    auto balls = ecs::View<>(getEcs()).with<Ball>();
    return balls.getCount() > 0;
}

void render_paddle()
{
    for (auto& [id, pos, size, paddle] : ecs::View<Position, Size, Paddle>(getEcs()))
    {
        render_rect(pos, size, sf::Color(50, 50, 200));
    }

}

void render_balls()
{
    for (auto& [id, pos, ball] : ecs::View<Position, Ball>(getEcs()))
    {
        render_circle(pos, ballRadius, sf::Color(100, 100, 100));
    }
}

void render_bricks()
{
    for (auto& [id, pos, size, brick] : ecs::View<Position, Size, Brick>(getEcs()))
    {
        render_rect(pos, size, brick.type == Brick::Type::Simple ? sf::Color::Magenta : sf::Color::Yellow);
    }
}

void render_tile_debug()
{
    ecs::entityId ballId = 0;
    auto& ecs = getEcs();
    for (auto& [id, ball] : ecs::View<Ball>(ecs))
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
            vec tileCenter(x * tileSize + tileSize * 0.5f, y * tileSize + tileSize * 0.5f);
            render_rect(tileCenter, vec(tileSize, tileSize) * 0.9f, includesBall ? sf::Color(0, 255, 0, 120) : sf::Color(255, 0, 0, 60));
        }
    }
}

void update()
{
    EASY_FUNCTION();
    update_paddle_by_mouse();
    update_balls_attached_to_paddle();

    update_positions_by_velocities();
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
    {
        update_ball_collisions();
    }
    else
    {
        update_ball_collisions_tiled();
    }
    resolve_ball_collisions();
    handle_brick_collisions();

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

#if 0
void update()
{
    vec mousePosition{sf::Mouse::getPosition(g_Globals.window)};
    mousePosition.y = g_Globals.screenSize.y - mousePosition.y;
    update_paddle(&g_Globals.gameState.paddle, mousePosition);
    if (g_debugBall)
    {
        update_ball_debug(&g_Globals.gameState.balls[0], mousePosition);
    }
    else
    {
        for(auto& it: g_Globals.gameState.balls)
        {
            update_ball(it.second);
        }
    }

	deleteEntitiesFromMemory();
}


#endif

void render()
{
    EASY_FUNCTION();
    render_paddle();
    render_balls();
    render_bricks();
}

sf::Text text;
sf::Font font;

void render_stats()
{
    EASY_FUNCTION();
    char debugString[1024];
    sprintf_s(debugString, "Frame time: %0.2f ms", g_Globals.elapsedTime * 1000.0f);
    text.setFont(font);
    text.setPosition(10, 10);
    text.setFillColor(sf::Color::Green);
    text.setString(debugString);
    g_Globals.window.draw(text);
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

    spawn_ball_on_paddle();

    sf::Clock clock;

    // run the program as long as the window is open
    while (window.isOpen())
    {
        sf::Time elapsedTime = clock.restart();
        g_Globals.elapsedTime = elapsedTime.asSeconds();
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
        render_stats();

        {
            EASY_BLOCK("Display");
            // end the current frame
            window.display();
        }
    }

    return 0;
}
