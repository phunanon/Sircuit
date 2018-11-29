#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

struct ObjX;

class Obj : public sf::Shape
{
public:
	sf::Vector2f Size = sf::Vector2f(10, 10);
	sf::Vector2f RoughOffset = sf::Vector2f(0, 0);
	sf::Vector2f RoughSize = sf::Vector2f(10, 10);
	sf::Vector2f CenterPos = sf::Vector2f(5, 5);
	uint8_t TypeId;
	uint16_t Id;
	uint16_t SaveId;
	bool Connectable = true;
	bool Electrified = false;
	bool StageElectrified = false;
	std::vector<ObjX*> Connections;
	bool SelectParticipant = false;
	
	Obj () {};
	~Obj ();
	void setLocation (sf::Vector2f);
	void move (sf::Vector2f);
	sf::FloatRect getRoughRect ();
	virtual bool getActive () = 0;
	virtual void Click () = 0;
	virtual const wchar_t* getString() = 0;
	virtual Obj* Clone() const = 0;
	bool operator==(const Obj& o) const { return this == &o; }
protected:
	sf::VertexArray* _vertices = NULL;
	sf::VertexArray* MakeOctogon (sf::Color, sf::Vector2f);
	sf::VertexArray* MakeResizeRectangle (sf::Color, sf::Vector2f);
	sf::VertexArray* MakeSmallDiamond (sf::Color, sf::Vector2f);
	sf::VertexArray* MakeSmallSquare (sf::Color, sf::Vector2f);
	void Recolor (sf::Color);
	bool actOR ();
private:
	virtual void draw (sf::RenderTarget& target, sf::RenderStates states) const
	{
		auto transform = sf::Transform();
		transform.translate(getPosition());
		transform.rotate(getRotation(), sf::Vector2f(Size.x/2, Size.y/2));
		states.transform.combine(transform);
		target.draw(*_vertices, states);
	}
	virtual std::size_t getPointCount () const { return _vertices->getVertexCount(); }
	virtual sf::Vector2f getPoint (std::size_t i) const { return ((*_vertices)[i]).position; }
};




class sfLine : public sf::Drawable
{
private:
    sf::VertexArray _vertices;
    float _thickness;
    sf::Color _color1;
    sf::Color _color2;
public:
	void setPositions (sf::Vector2f p1, sf::Vector2f p2)
	{
		sf::Vector2f direction = p2 - p1;
        sf::Vector2f unitDirection = direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);
        sf::Vector2f unitPerpendicular(-unitDirection.y, unitDirection.x);
        sf::Vector2f offset = (_thickness / 2.f) * unitPerpendicular;

		_vertices = sf::VertexArray(sf::PrimitiveType::Quads, 4);
        _vertices[0] = sf::Vertex(p1 + offset, _color1);
        _vertices[1] = sf::Vertex(p2 + offset, _color2);
        _vertices[2] = sf::Vertex(p2 - offset, _color2);
        _vertices[3] = sf::Vertex(p1 - offset, _color1);
	}
	
    sfLine(sf::Color color1, sf::Color color2, float thickness, sf::Vector2f p1, sf::Vector2f p2) : _color1(color1), _color2(color2), _thickness(thickness)
    {
        setPositions(p1, p2);
    }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const
    {
        target.draw(_vertices, states);
    }
    
    void ChangeHostColor (sf::Color hostColor)
    {
    	_vertices[0].color = hostColor;
    	_vertices[3].color = hostColor;
    }
};

struct ObjX
{
	uint16_t ForeignId;
	Obj* Host;
	Obj* Foreign;
	bool IsHost;
	sfLine* Line;
	void RebuildLine () { Line = new sfLine(sf::Color::White, sf::Color::Black, 1, Host->CenterPos, Foreign->CenterPos); }
	ObjX (Obj* host, Obj* foreign, bool isHost, uint16_t foreignId = 0) : Host(host), Foreign(foreign), IsHost(isHost), ForeignId(foreignId)
	{
		if (isHost && host && foreign) RebuildLine();
	};
	bool ContainsPoint (sf::Vector2f point);
};



class ObjAND : public Obj
{
public:
	ObjAND ();
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L"&"; };
	Obj* Clone () const { return new ObjAND(*this); }
};

class ObjOR : public Obj
{
public:
	ObjOR ();
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L"•"; }
	Obj* Clone () const { return new ObjOR(*this); }
};

class ObjXOR : public Obj
{
public:
	ObjXOR ();
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L"⊕"; }
	Obj* Clone () const { return new ObjXOR(*this); }
};

class ObjNOT : public Obj
{
public:
	ObjNOT ();
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L"~"; }
	Obj* Clone () const { return new ObjNOT(*this); }
};

class ObjSwitch : public Obj
{
public:
	bool IsOn = false;
	ObjSwitch ();
	void Click ();
	bool getActive ();
	const wchar_t* getString () { return IsOn ? L"⭘" : L"⏻"; }
	Obj* Clone () const { return new ObjSwitch(*this); }
};

class ObjPanel : public Obj
{
public:
	bool Resizing = false;
	ObjPanel ();
	void RemakeRect ();
	void Resize (sf::Vector2f to) { Size = to; RemakeRect(); }
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L" "; }
	Obj* Clone () const { return new ObjPanel(*this); }
};

class ObjRandom : public Obj
{
public:
	ObjRandom ();
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L"⚂"; }
	Obj* Clone () const { return new ObjRandom(*this); }
};

class ObjBit : public Obj
{
public:
	bool IsOn = false;
	ObjBit ();
	void Click ();
	bool getActive ();
	const wchar_t* getString () { return StageElectrified ? L"1" : L"0"; }
	Obj* Clone () const { return new ObjBit(*this); }
};

class ObjIndicator : public Obj
{
public:
	bool prevActive = true;
	ObjIndicator ();
	void Click () {};
	bool getActive ();
	const wchar_t* getString () { return L" "; }
	Obj* Clone () const { return new ObjIndicator(*this); }
};


