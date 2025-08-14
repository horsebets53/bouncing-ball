#include <SFML/Graphics.hpp>
#include <cmath>
#include <deque>
#include <cstdint>
#include <algorithm>
#include "Ball.hpp"
// [RU] класс с shape и velocity
// [EN] class with shape and velocity

// ───────────────────────────────────────────────────────────────────────────────
// [RU] КОНФИГУРАЦИЯ ОКНА
// [EN] WINDOW CONFIGURATION
constexpr unsigned WINDOW_W   = 800;
constexpr unsigned WINDOW_H   = 800;
constexpr unsigned MSAA       = 16;
constexpr auto     WINDOW_TITLE = "SFML 800x800";
constexpr auto     WINDOW_STYLE = sf::Style::Default;
constexpr auto     WINDOW_STATE = sf::State::Windowed;
const     sf::Color CLEAR_COLOR = sf::Color::Black;

// [RU] БОЛЬШАЯ ОКРУЖНОСТЬ
// [EN] BIG CIRCLE
constexpr float  BIG_RADIUS          = 350.f;
constexpr size_t BIG_SEGMENTS        = 256;
constexpr float  BIG_OUTLINE_THICK   = 5.f;
const     sf::Color BIG_OUTLINE_COLOR = sf::Color::White;

// [RU] ШАРИК (начальные параметры)
// [EN] BALL (initial parameters)
constexpr float  BALL_RADIUS0   = 20.f;
constexpr size_t BALL_SEGMENTS  = 128;
const     sf::Color BALL_COLOR  = sf::Color::Red;
const     sf::Vector2f BALL_START_POS = {WINDOW_W * 0.5f, 200.f};
const     sf::Vector2f BALL_START_VEL = {120.f, 0.f};

// [RU] ФИЗИКА
// [EN] PHYSICS
constexpr float SIM_SPEED   = 1.0f;   // [RU] 1.0 = нормальная скорость
                                      // [EN] 1.0 = normal speed
constexpr float GRAVITY     = 800.0f; // [RU] пикс/с^2 вниз
                                      // [EN] px/s^2 downward

// [RU] Рост шарика при столкновении
// [EN] Ball growth on collision
constexpr bool  GROW_ON_HIT  = false;  // [RU] вкл/выкл рост радиуса при ударе
                                       // [EN] enable/disable radius growth on impact
constexpr float GROW_PER_HIT = 2.0f;   // [RU] +радиус за удар
                                       // [EN] +radius per hit
constexpr float RADIUS_MARGIN = 2.0f;  // [RU] зазор до стенки после роста
                                       // [EN] clearance to the wall after growth

// [RU] ХВОСТ
// [EN] TRAIL
constexpr std::size_t TRAIL_MAX      = 250; // [RU] длина хвоста (позиций)
                                            // [EN] trail length (positions)
constexpr float       TRAIL_ALPHA_MIN = 0.00f;
constexpr float       TRAIL_ALPHA_MAX = 0.05f; // [RU] «очень прозрачный» хвост
                                               // [EN] "very transparent" trail
constexpr float       TRAIL_GAMMA     = 3.0f;  // [RU] >1 — старые слабее
                                               // [EN] >1 — older ones are weaker
constexpr float       HUE_SPEED       = 60.f;  // [RU] °/сек — скорость радуги
                                               // [EN] deg/s — rainbow speed
// ───────────────────────────────────────────────────────────────────────────────

// [RU] Вспомогательная математика
// [EN] Helper math
static inline float dot(const sf::Vector2f& a, const sf::Vector2f& b) {
    return a.x * b.x + a.y * b.y;
}
static inline float length(const sf::Vector2f& v) {
    return std::sqrt(dot(v, v));
}
static inline sf::Vector2f normalize(const sf::Vector2f& v) {
    float L = length(v);
    return (L > 0.f) ? sf::Vector2f{v.x / L, v.y / L} : sf::Vector2f{1.f, 0.f};
}

// HSV → RGB (h∈[0,360), s,v∈[0,1], a∈[0,255])
inline sf::Color HSVtoRGB(float h, float s, float v, std::uint8_t a = 255) {
    h = std::fmod(std::fmod(h, 360.f) + 360.f, 360.f);
    s = std::clamp(s, 0.f, 1.f);
    v = std::clamp(v, 0.f, 1.f);

    float r = v, g = v, b = v;
    if (s > 0.f) {
        float H = h / 60.f;
        int   i = static_cast<int>(std::floor(H));
        float f = H - i;
        float p = v * (1.f - s);
        float q = v * (1.f - s * f);
        float t = v * (1.f - s * (1.f - f));
        switch ((i % 6 + 6) % 6) {
            case 0: r=v; g=t; b=p; break;
            case 1: r=q; g=v; b=p; break;
            case 2: r=p; g=v; b=t; break;
            case 3: r=p; g=q; b=v; break;
            case 4: r=t; g=p; b=v; break;
            case 5: r=v; g=p; b=q; break;
        }
    }
    auto to8 = [](float x){ return static_cast<std::uint8_t>(std::lround(255.f * x)); };
    return sf::Color(to8(r), to8(g), to8(b), a);
}

int main() {
    // [RU] Окно + MSAA
    // [EN] Window + MSAA
    sf::ContextSettings settings;
    settings.antiAliasingLevel = MSAA;

    sf::RenderWindow window(
        sf::VideoMode({WINDOW_W, WINDOW_H}),
        WINDOW_TITLE,
        WINDOW_STYLE,
        WINDOW_STATE,
        settings
    );
    const sf::Vector2f CENTER{WINDOW_W * 0.5f, WINDOW_H * 0.5f};

    // [RU] Большая окружность
    // [EN] Big circle
    sf::CircleShape big(BIG_RADIUS, BIG_SEGMENTS);
    big.setFillColor(sf::Color::Transparent);
    big.setOutlineColor(BIG_OUTLINE_COLOR);
    big.setOutlineThickness(BIG_OUTLINE_THICK);
    big.setOrigin({BIG_RADIUS, BIG_RADIUS});
    big.setPosition(CENTER);

    // [RU] Шарик
    // [EN] Ball
    Ball ball(BALL_RADIUS0, BALL_START_POS, BALL_START_VEL, BALL_SEGMENTS);
    ball.shape.setFillColor(BALL_COLOR);

    // [RU] Хвост
    // [EN] Trail
    std::deque<sf::Vector2f> trail;
    sf::CircleShape ghost(ball.shape.getRadius(), BALL_SEGMENTS);
    ghost.setOrigin({ball.shape.getRadius(), ball.shape.getRadius()});
    ghost.setOutlineThickness(0.f);

    sf::Clock clock;
    float hueShift = 0.f;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
        }

        float dt = clock.restart().asSeconds() * SIM_SPEED;
        hueShift = std::fmod(hueShift + HUE_SPEED * dt, 360.f);

        // [RU] Гравитация
        // [EN] Gravity
        ball.velocity += sf::Vector2f{0.f, GRAVITY} * dt;

        // [RU] Интеграция
        // [EN] Integration
        ball.update(dt);

        // [RU] Коллизия с внутренней стенкой большой окружности
        // [EN] Collision with the inner wall of the big circle
        const sf::Vector2f C = big.getPosition();
        const sf::Vector2f P = ball.shape.getPosition();

        sf::Vector2f CP = P - C;
        const float r   = ball.shape.getRadius();
        const float Rin = BIG_RADIUS - r; // [RU] допустимый радиус центра
                                          // [EN] permissible radius for the center
        float d = length(CP);

        if (d > Rin) {
            sf::Vector2f n = normalize(CP);

            // [RU] вернуть на границу
            // [EN] snap back to the boundary
            ball.shape.setPosition(C + n * Rin);

            // [RU] отражение
            // [EN] reflection
            float vn = dot(ball.velocity, n);
            if (vn > 0.f) {
                ball.velocity = ball.velocity - 2.f * vn * n;

                // [RU] рост радиуса (опционально)
                // [EN] radius growth (optional)
                if (GROW_ON_HIT) {
                    float maxR = BIG_RADIUS - RADIUS_MARGIN;
                    float newR = std::min(r + GROW_PER_HIT, maxR);
                    if (newR > r) {
                        ball.shape.setRadius(newR);
                        ball.shape.setOrigin({newR, newR});
                        // [RU] снова прижать к границе с новым радиусом
                        // [EN] clamp to the boundary again with the new radius
                        const float Rin2 = BIG_RADIUS - newR;
                        ball.shape.setPosition(C + n * Rin2);
                    }
                }
            }
        }

        // [RU] обновление хвоста
        // [EN] update the trail
        trail.push_back(ball.shape.getPosition());
        if (trail.size() > TRAIL_MAX) trail.pop_front();

        // [RU] рендер
        // [EN] render
        window.clear(CLEAR_COLOR);

        // [RU] подстроить «призрак» под текущий радиус
        // [EN] adjust the "ghost" to the current radius
        ghost.setRadius(ball.shape.getRadius());
        ghost.setOrigin({ghost.getRadius(), ghost.getRadius()});

        // [RU] хвост (от старых к новым)
        // [EN] trail (from oldest to newest)
        const std::size_t N = trail.size();
        for (std::size_t i = 0; i < N; ++i) {
            float t = (N > 1) ? static_cast<float>(i) / (N - 1) : 1.f; // [RU] 0..1
                                                                       // [EN] 0..1
            float a01 = TRAIL_ALPHA_MIN +
                        (TRAIL_ALPHA_MAX - TRAIL_ALPHA_MIN) * std::pow(t, TRAIL_GAMMA);
            std::uint8_t alpha = static_cast<std::uint8_t>(std::lround(255.f * a01));

            float hue = std::fmod(hueShift + 360.f * t, 360.f);
            ghost.setFillColor(HSVtoRGB(hue, 1.f, 1.f, alpha));
            ghost.setPosition(trail[i]);
            window.draw(ghost);
        }

        window.draw(big);
        window.draw(ball.shape);
        window.display();
    }
}
