#include "Common.hpp"

sf::Vector2f vabs (sf::Vector2f v) { return sf::Vector2f(abs(v.x), abs(v.y)); }
sf::Vector2f vround (sf::Vector2f v, float r) { return sf::Vector2f(std::round(v.x / r), std::round(v.y / r)) * r; }
