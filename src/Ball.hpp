#pragma once
#include <SFML/Graphics.hpp>

class Ball {
public:
    sf::CircleShape shape;
    sf::Vector2f    velocity{};

    Ball(float radius,
         const sf::Vector2f& startPos,
         const sf::Vector2f& startVel,
         std::size_t segments = 128)
        : shape(radius, segments), velocity(startVel)
    {
        shape.setOrigin({radius, radius});
        shape.setPosition(startPos);
        // Никаких setFillColor/outline здесь — стиль задаём в main.cpp
    }

    // Удобный метод: менять радиус и сразу корректно обновлять origin
    void setRadius(float r) {
        shape.setRadius(r);
        shape.setOrigin({r, r});
    }

    void update(float dt) {
        shape.move(velocity * dt);
    }
};
