#include "break.h"
#include "vec.h"
#include "collision.h"

const float paddleHeight = 25.0f;
const float ballRadius = 20.0f;
const float ballStartingSpeed = 800.0f;

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

void init_globals()
{
    g_Globals.gameState.paddle.position.y = paddleHeight * 0.5f;

    g_Globals.rectPrototype.setSize(vec{ 1.0f, 1.0f });
    g_Globals.rectPrototype.setOrigin(vec{0.5f, 0.5f});
    g_Globals.rectPrototype.setFillColor(sf::Color::White);

    g_Globals.circlePrototype.setRadius(1.0f);
    g_Globals.circlePrototype.setFillColor(sf::Color(100, 100, 100));
}

void setup_level(GameState& gamestate)
{
    int rows = 15;
    int columns = 10;

    float brickSpacing = 5;

    gamestate.bricks.clear();
    gamestate.bricks.reserve(rows*columns);

    vec originalBrickPosition = vec{ 50, 550 };
    vec currentBrickPos = originalBrickPosition;
    vec brickSize = vec{ 60, 20 };

    for(int i = 0; i < columns; i++)
    {
        for(int j = 0; j < rows; j++)
        {
            gamestate.bricks.push_back(Brick{ currentBrickPos, brickSize });
            currentBrickPos.x += brickSize.x + brickSpacing;
        }
        currentBrickPos.x = originalBrickPosition.x;
        currentBrickPos.y -= brickSize.y + brickSpacing;
    }
}

void place_ball_on_paddle(Ball* ball, Paddle* paddle)
{
    ball->position.x = paddle->position.x;
    ball->position.y = paddle->position.y + paddleHeight * 0.5f + ballRadius;
    ball->waitingToBeFired = true;
}

void fire_ball(Ball* ball)
{
    if (ball->waitingToBeFired)
    {
        ball->waitingToBeFired = false;
        ball->velocity = vec_normalize(vec{1,1}) * ballStartingSpeed;
    }
}

void update_paddle(Paddle* paddle, vec mousePosition)
{
    paddle->position.x = mousePosition.x;
    paddle->position.x = std::max(paddle->position.x, paddle->size * 0.5f);
    paddle->position.x = std::min(paddle->position.x, g_Globals.screenSize.x - paddle->size * 0.5f);
}

void update_ball_debug(Ball* ball, vec mousePosition)
{
    ball->position = mousePosition;
    ball->velocity = vec{ 0, 0 };   
}

void render_debug()
{
    Ball* ball = &g_Globals.gameState.ball;
    for (int i = 0; i < g_Globals.gameState.bricks.size(); i++)
    {
        auto& brick = g_Globals.gameState.bricks[i];
        AabbOverlapResult result;
        bool overlap = test_circle_aabb_overlap(result, ball->position, ballRadius, brick.position - brick.size * 0.5f, brick.position + brick.size * 0.5f, false);
        if (overlap)
        {
            render_line(result.collisionPoint, result.collisionPoint + result.normal * 100.0f);
        }
    }
}

bool is_ball_under_screen(Ball* ball)
{
    AabbOverlapResult overlapDetails;
    bool overlap = test_circle_aabb_overlap(overlapDetails, ball->position, ballRadius, vec{ 0, -1000 }, vec{ g_Globals.screenSize.x, 0 }, true);
    return overlap;
}

void collision_ball(Ball* ball)
{
    for (int i = 0; i < g_Globals.gameState.bricks.size(); i++)
    {
        auto& brick = g_Globals.gameState.bricks[i];
        AabbOverlapResult result;
        bool overlap = test_circle_aabb_overlap(result, ball->position, ballRadius, brick.position - brick.size * 0.5f, brick.position + brick.size * 0.5f, false);
        if (overlap)
        {
            ball->velocity = vec_reflect(ball->velocity, result.normal);
            unordered_delete(g_Globals.gameState.bricks, i);
            i--;
        }
    }
}

void update_ball(Ball* ball)
{
    auto paddle = &g_Globals.gameState.paddle;

    if (ball->waitingToBeFired)
    {
        place_ball_on_paddle(ball, paddle);
        return;
    }

    collision_ball(ball);

    // TODO simulate collisions correctly by taking the collision time and resimulating the rest of the time with the altered velocity
    ball->position += ball->velocity * g_Globals.clock.getElapsedTime().asSeconds();

    if (ball->position.x - ballRadius < 0)
    {
        ball->position.x = ballRadius;
        ball->velocity.x *= -1.0f;
    }
    else if (ball->position.x + ballRadius > g_Globals.screenSize.x)
    {
        ball->position.x = g_Globals.screenSize.x - ballRadius;
        ball->velocity.x *= -1.0f;
    }
    
    if (ball->position.y + ballRadius > g_Globals.screenSize.y)
    {
        ball->position.y = g_Globals.screenSize.y - ballRadius;
        ball->velocity.y *= -1.0f;
    }

    float paddleTop = paddle->position.y + paddleHeight * 0.5f;

    if(is_ball_under_screen(ball))
    {
        place_ball_on_paddle(ball, paddle);
    }
    else if(ball->velocity.y < 0.0f && ball->position.y - ballRadius < paddleTop && ball->position.y > paddleTop)
    {
        // The AABB of the ball overlaps with that of the paddle. Check for circle - segment collision.
        vec paddleTopLeft = paddle->position + vec(-paddle->size * 0.5f, paddleHeight * 0.5f);
        SegmentOverlapResult ballPaddleOverlap = test_circle_segment_overlap(ball->position, ballRadius, paddleTopLeft, paddleTopLeft + vec(paddle->size, paddleHeight));

        if(ballPaddleOverlap.overlapped)
        {
            ball->velocity.y = 1;
            ball->velocity.x = lerp(-1, 1, ballPaddleOverlap.collisionParam);
            ball->velocity = vec_normalize(ball->velocity) * ballStartingSpeed;
        }
    }
}

void render_paddle(Paddle* paddle)
{
    vec paddleSize{paddle->size, paddleHeight};
    render_rect(paddle->position, paddleSize, sf::Color(50, 50, 200));
}

void render_ball(Ball* ball)
{
    render_circle(ball->position, ballRadius, sf::Color(100, 100, 100));
}

void update()
{
    vec mousePosition{sf::Mouse::getPosition(g_Globals.window)};
    mousePosition.y = g_Globals.screenSize.y - mousePosition.y;
    update_paddle(&g_Globals.gameState.paddle, mousePosition);
    if (g_debugBall)
    {
        update_ball_debug(&g_Globals.gameState.ball, mousePosition);
    }
    else
    {
        update_ball(&g_Globals.gameState.ball);
    }
}

void render_bricks(const std::vector<Brick>& bricks)
{
    for (auto& brick : bricks)
    {
        render_rect(brick.position, brick.size, sf::Color::Magenta);
    }
}

void render()
{
    render_paddle(&g_Globals.gameState.paddle);
    render_ball(&g_Globals.gameState.ball);
    render_bricks(g_Globals.gameState.bricks);

    if (g_debugBall)
    {
        render_debug();
    }
}

int main()
{
    init_globals();
    setup_level(g_Globals.gameState);

    auto& window = g_Globals.window;
    window.setMouseCursorVisible(false);

    place_ball_on_paddle(&g_Globals.gameState.ball, &g_Globals.gameState.paddle);

    // run the program as long as the window is open
    while (window.isOpen())
    {
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
                    fire_ball(&g_Globals.gameState.ball);
                }
            }
        }

        // clear the window with black color
        window.clear(sf::Color::Blue);

        update();
        render();
        // end the current frame
        window.display();

        g_Globals.clock.restart();
    }

    return 0;
}
