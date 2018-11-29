#pragma once
#include <SFML/Graphics.hpp>
#include "Object.hpp"

class Display
{
private:
	sf::RenderWindow* _window;
	sf::Font _font;
	sf::Text _text;
	sf::CircleShape _origin;
	sf::VertexArray _grid;
public:
	Display (sf::RenderWindow& window);
	void Render (sf::Transform&, std::vector<Obj*>&, Obj* connObj, sf::Vector2f mLoc);
};
