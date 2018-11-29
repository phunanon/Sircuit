#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

template<class T>
std::ostream& operator<< (std::ostream& os, sf::Vector2<T> v)
{
	os << f(v.x) <<","<< f(v.y);
	return os;
}

template<class T>
float dist (sf::Vector2<T> a, sf::Vector2<T>b) { return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));}

template<class T>
sf::Vector2<T> operator*(sf::Vector2<T> a, sf::Vector2<T> b) { return sf::Vector2<T>(a.x * b.x, a.y * b.y); }
template<class T>
sf::Vector2i operator*(sf::Vector2i a, int b) { return sf::Vector2i(a.x * b, a.y * b); }
template<class T>
sf::Vector2<T> operator/(sf::Vector2<T> a, sf::Vector2<T> b) { return sf::Vector2<T>(a.x / b.x, a.y / b.y); }
template<class T>
sf::Vector2<T> operator/(sf::Vector2<T> a, int b) { return sf::Vector2<T>(a.x / b, a.y / b); }
template<class T>
sf::Vector2<T> operator/(int a, sf::Vector2<T> b) { return sf::Vector2<T>(a / b.x, a / b.y); }
template<class T>
sf::Vector2<T> operator-(sf::Vector2<T> a, float b) { return sf::Vector2<T>(a.x - b, a.y - b); }

sf::Vector2f vabs (sf::Vector2f v);
sf::Vector2f vround (sf::Vector2f v, float r);
