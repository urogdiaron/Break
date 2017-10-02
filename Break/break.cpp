#include "break.h"

const float paddleHeight = 25.0f;
const float ballRadius = 100.0f;
const float ballStartingSpeed = 3500.0f;

std::unique_ptr<Globals> g_Globals;

float vec_dot(vec v1, vec v2)
{
    return v1.x * v2.x + v1.y * v2.y;
}

float vec_lengthsq(vec v)
{
    return vec_dot(v, v);
}

float vec_length(vec v)
{
    return std::sqrt(vec_lengthsq(v));
}

vec vec_normalize(vec v)
{
    return v / vec_length(v);
}

float lerp(float from, float to, float t)
{
    return from + t * (to - from);
}

void render_rect(vec topLeft, vec scale)
{
    topLeft.y = g_Globals->screenSize.y - topLeft.y;

    sf::RenderStates renderState;
    renderState.transform.translate(topLeft).scale(scale);
    g_Globals->window.draw(g_Globals->rectPrototype, renderState);
}

void render_circle(vec center, float radius)
{
    center.y = g_Globals->screenSize.y - center.y;
    center.x -= radius;
    center.y -= radius;
    sf::RenderStates renderState;
    renderState.transform.translate(center).scale(radius, radius);
    g_Globals->window.draw(g_Globals->circlePrototype, renderState);
}

void init_globals()
{
    g_Globals = std::make_unique<Globals>();

    g_Globals->gameState.paddle.position.y = paddleHeight * 0.5f;

    g_Globals->rectPrototype.setSize(vec{1.0f, 1.0f});
    g_Globals->rectPrototype.setFillColor(sf::Color::White);

    g_Globals->circlePrototype.setRadius(1.0f);
    g_Globals->circlePrototype.setFillColor(sf::Color(100,100,100));
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
    paddle->position.x = std::min(paddle->position.x, g_Globals->screenSize.x - paddle->size * 0.5f);
}

void update_ball(Ball* ball)
{
    auto paddle = &g_Globals->gameState.paddle;

    if (ball->waitingToBeFired)
    {
        place_ball_on_paddle(ball, paddle);
        return;
    }

    // TODO simulate collisions correctly by taking the collision time and resimulating the rest of the time with the altered velocity
    ball->position += ball->velocity * g_Globals->clock.getElapsedTime().asSeconds();

    if (ball->position.x - ballRadius < 0)
    {
        ball->position.x = ballRadius;
        ball->velocity.x *= -1.0f;
    }
    else if (ball->position.x + ballRadius > g_Globals->screenSize.x)
    {
        ball->position.x = g_Globals->screenSize.x - ballRadius;
        ball->velocity.x *= -1.0f;
    }
    
    if (ball->position.y + ballRadius > g_Globals->screenSize.y)
    {
        ball->position.y = g_Globals->screenSize.y - ballRadius;
        ball->velocity.y *= -1.0f;
    }

    float paddleTop = paddle->position.y + paddleHeight * 0.5f;

    if(ball->position.y < 0.0f)
    {
        place_ball_on_paddle(ball, paddle);
    }
    else if(ball->velocity.y < 0.0f && ball->position.y - ballRadius < paddleTop && ball->position.y > paddleTop)
    {
        // The AABB of the ball overlaps with that of the paddle. Check for circle - segment collision.
        vec paddleTopLeft = paddle->position + vec(-paddle->size * 0.5f, paddleHeight * 0.5f);
        vec toPaddle = paddleTopLeft - ball->position;

        float a = paddle->size * paddle->size;
        float b = 2 * toPaddle.x * paddle->size;
        float c = vec_dot(toPaddle, toPaddle) - ballRadius * ballRadius;

        float collisionParam = -1.0f;
        float discriminant = b*b - 4 * a*c;
        if (discriminant >= 0)
        {
            discriminant = sqrt(discriminant);
            float t1 = (-b - discriminant) / (2 * a);
            float t2 = (-b + discriminant) / (2 * a);
            if (t1 >= 0 && t1 <= 1)
            {
                collisionParam = t1;
            }
            if (t2 >= 0 && t2 <= 1)
            {
                collisionParam = t2;
            }
        }

        if(collisionParam >= 0)
        {
            ball->velocity.y = 1;
            ball->velocity.x = lerp(-1, 1, collisionParam);
            ball->velocity = vec_normalize(ball->velocity) * ballStartingSpeed;
        }
    }
}

void render_paddle(Paddle* paddle)
{
    vec paddleSize{paddle->size, paddleHeight};
    render_rect(vec{ paddle->position.x - paddleSize.x * 0.5f, paddle->position.y + paddleSize.y * 0.5f }, paddleSize);
}

void render_ball(Ball* ball)
{
    render_circle(ball->position, ballRadius);
}

void update()
{
    vec mousePosition{sf::Mouse::getPosition(g_Globals->window)};
    update_paddle(&g_Globals->gameState.paddle, mousePosition);
    update_ball(&g_Globals->gameState.ball);
}

void render()
{
    render_paddle(&g_Globals->gameState.paddle);
    render_ball(&g_Globals->gameState.ball);
}

int main()
{
    init_globals();

    auto& window = g_Globals->window;
    window.setMouseCursorVisible(false);

    place_ball_on_paddle(&g_Globals->gameState.ball, &g_Globals->gameState.paddle);

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
                if (event.key.code == sf::Keyboard::Escape)
                {
                    window.close();
                }
            }
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    fire_ball(&g_Globals->gameState.ball);
                }
            }
        }

        // clear the window with black color
        window.clear(sf::Color::Blue);

        update();
        render();
        // end the current frame
        window.display();

        g_Globals->clock.restart();
    }

    return 0;
}
