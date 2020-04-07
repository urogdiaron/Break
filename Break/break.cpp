#include "break.h"

const float paddleHeight = 25.0f;
const float ballRadius = 20.0f;
const float ballStartingSpeed = 400.0f;

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
    ecs.registerType<Paddle>("Paddle");
    ecs.registerType<AttachedToPaddle>("AttachedToPaddle");
    ecs.registerType<Ball>("Ball");
    ecs.registerType<Brick>("Brick");        
}

void setup_level(GameState& gamestate)
{
    auto& ecs = getEcs();

    // Create the paddle
    entityId paddleId = ecs.createEntity<Position, Size, Paddle>(Position{ g_Globals.screenSize.x * 0.5f, paddleHeight * 0.5f }, Size{ 150.0f, paddleHeight }, Paddle{});

    int rows = 15;
    int columns = 15;

    float brickSpacing = 5;

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

            ecs.createEntity<Position, Size, Brick>(Position{ currentBrickPos }, Size{ brickSize }, Brick{brickType});
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

    auto& [id, pos, size, paddle] = *itPaddle;
    vec ballPosition = pos + vec(0.0f, size.y * 0.5f + ballRadius);

    ecs.createEntity<Position, Velocity, Ball, AttachedToPaddle>(
        Position{ ballPosition }, 
        Velocity{}, 
        Ball{}, 
        AttachedToPaddle{ id, vec(0.0f, size.y * 0.5f + ballRadius) 
    });
}

void fire_ball()
{
    ecs::View<Ball, AttachedToPaddle, Velocity> view(getEcs());
    for (auto& [id, ball, attach, vel] : view)
    {
        (vec&)vel = vec_normalize(vec{ 1,1 }) * ballStartingSpeed;
        view.deleteComponents(id, ecs::getTypes<AttachedToPaddle>());
    }
}

void update_positions_by_velocities()
{
    float dt = g_Globals.elapsedTime;
    for (auto& [id, pos, vel] : ecs::View<Position, Velocity>(getEcs()))
    {
        pos += vel * dt;
    }
}

void update_ball_collisions()
{
    g_Globals.ballCollisions.clear();
    ecs::View<Position, Ball> pos_ball(getEcs());
    for (auto& [ballId, ballPos, ball] : pos_ball)
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

        for (auto& [paddleId, paddlePos, paddleSize, paddle] : ecs::View<Position, Size, Paddle>(getEcs()))
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
        auto [pos, vel] = ecs.getComponents<Position, Velocity>(ballCollision.ballId);
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
    auto brickView = ecs::View<Position, Size, Brick>(ecs);
    for (auto [brickId, pos, size, brick] : brickView)
    {
        if (brick.wasHitThisFrame)
        {
            if (brick.type == Brick::Type::Ballspawner)
            {
                vec ballPosition = pos - vec(0, size.y + ballRadius);
                vec ballVelocity = vec(0, -ballStartingSpeed);
                brickView.createEntity<Position, Velocity, Ball>(Position{ ballPosition }, Velocity{ ballVelocity }, Ball{});
            }
            brick.wasHitThisFrame = false;
            brickView.deleteEntity(brickId);
        }
    }
}

bool has_balls_in_play()
{
    ecs::View<Ball> balls(getEcs());
    return balls.getCount() > 0;
}

#if 0
void update_ball_debug(Ball* ball, vec mousePosition)
{
    ball->position = mousePosition;
    ball->velocity = vec{ 0, 0 };   
}

void render_debug()
{
    Ball* ball = &g_Globals.gameState.balls.begin()->second;
    for (auto& it : g_Globals.gameState.bricks)
    {
		auto& brick = it.second;
        OverlapResult result;
        bool overlap = test_circle_aabb_overlap(result, ball->position, ballRadius, brick.position - brick.size * 0.5f, brick.position + brick.size * 0.5f, false);
        if (overlap)
        {
            render_line(result.collisionPoint, result.collisionPoint + result.normal * 100.0f);
        }
    }
}

bool is_ball_under_screen(Ball& ball)
{
    OverlapResult overlapDetails;
    bool overlap = test_circle_aabb_overlap(overlapDetails, ball.position, ballRadius, vec{ 0, -1000 }, vec{ g_Globals.screenSize.x, 0 }, true);
    return overlap;
}

void collision_ball(Ball& ball)
{
    auto& bricks = g_Globals.gameState.bricks;
    for (auto& it : g_Globals.gameState.bricks)
    {
		auto& brick = it.second;
        OverlapResult result;
        bool overlap = test_circle_aabb_overlap(result, ball.position, ballRadius, brick.position - brick.size * 0.5f, brick.position + brick.size * 0.5f, false);
        if (overlap)
        {
            ball.velocity = vec_reflect(ball.velocity, result.normal);
			deleteEntity(it.first);
        }
    }
}

void update_ball(Ball& ball)
{
    Paddle& paddle = g_Globals.gameState.paddle;

    if (ball.holderPaddle)
    {
        place_ball_on_paddle(ball, paddle);
        return;
    }

    collision_ball(ball);

    // TODO simulate collisions correctly by taking the collision time and resimulating the rest of the time with the altered velocity
    ball.position += ball.velocity * g_Globals.clock.getElapsedTime().asSeconds();

    if (ball.position.x - ballRadius < 0)
    {
        ball.position.x = ballRadius;
        ball.velocity.x *= -1.0f;
    }
    else if (ball.position.x + ballRadius > g_Globals.screenSize.x)
    {
        ball.position.x = g_Globals.screenSize.x - ballRadius;
        ball.velocity.x *= -1.0f;
    }
    
    if (ball.position.y + ballRadius > g_Globals.screenSize.y)
    {
        ball.position.y = g_Globals.screenSize.y - ballRadius;
        ball.velocity.y *= -1.0f;
    }

    float paddleTop = paddle.position.y + paddleHeight * 0.5f;

    if(is_ball_under_screen(ball))
    {
        place_ball_on_paddle(ball, paddle);
    }
    else if(ball.velocity.y < 0.0f && ball.position.y - ballRadius < paddleTop && ball.position.y > paddleTop)
    {
        // The AABB of the ball overlaps with that of the paddle. Check for circle - segment collision.
        vec paddleTopLeft = paddle.position + vec(-paddle.size * 0.5f, paddleHeight * 0.5f);
        SegmentOverlapResult ballPaddleOverlap = test_circle_segment_overlap(ball.position, ballRadius, paddleTopLeft, paddleTopLeft + vec(paddle.size, paddleHeight));

        if(ballPaddleOverlap.overlapped)
        {
			vec normal{ 0, 1 };
			normal.x = lerp(-0.5f, 0.5f, ballPaddleOverlap.collisionParam);
			normal = vec_normalize(normal);
			ball.velocity = vec_reflect(ball.velocity, normal);
            ball.velocity = vec_normalize(ball.velocity) * ballStartingSpeed;
        }
    }
}
#endif

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

void update()
{
    EASY_FUNCTION();
    float dt = g_Globals.elapsedTime;
    auto& ecs = getEcs();

    // Move the paddle to the mouse
    vec mousePosition{ sf::Mouse::getPosition(g_Globals.window) };
    float screenSizeX = g_Globals.screenSize.x;
    for (auto& [id, pos, size, paddle] : ecs::View<Position, Size, Paddle>(ecs))
    {
        pos.x = mousePosition.x;
        pos.x = std::max(pos.x, size.x * 0.5f);
        pos.x = std::min(pos.x, screenSizeX - size.x * 0.5f);
    }

    // Update things attached to the paddle
    for (auto& [id, pos, attach] : ecs::View<Position, AttachedToPaddle>(ecs))
    {
        auto paddlePos = ecs.getComponent<Position>(attach.paddleId);
        if (!paddlePos)
            continue;
        (vec&)pos = *paddlePos + attach.relativePos;
    }

    update_positions_by_velocities();
    update_ball_collisions();
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
    //
    //if (g_debugBall)
    //{
    //    render_debug();
    //}
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
