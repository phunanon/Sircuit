#include "Object.hpp"

Obj::~Obj ()
{
	for (auto &con : Connections) delete con;
}

void Obj::setLocation (sf::Vector2f to)
{
	setPosition(to);
	CenterPos = getPosition() + sf::Vector2f(Size.x/2, Size.y/2);
	//Change connection positions
	for (auto &con : Connections) {
		if (con->IsHost) con->Line->setPositions(CenterPos, con->Foreign->CenterPos);
		else
			for (auto &fcon : con->Foreign->Connections)
				if (fcon->Foreign == this) {
					fcon->Line->setPositions(con->Foreign->CenterPos, CenterPos);
					break;
				}
	}
}

void Obj::move (sf::Vector2f by) { setLocation(getPosition() + by); }

void Obj::Recolor (sf::Color newColor)
{
	for (uint8_t v = 0, vLen = getPointCount(); v < vLen; ++v) (*_vertices)[v].color = newColor;
}


sf::FloatRect Obj::getRoughRect ()
{
	return sf::FloatRect(getPosition() + RoughOffset, RoughSize);
}


ObjAND::ObjAND ()
{
	TypeId = O_AND;
	_vertices = MakeOctogon(sf::Color(250, 128, 50), Size);
}

ObjOR::ObjOR ()
{
	TypeId = O_OR;
	_vertices = MakeSmallDiamond(sf::Color(150, 150, 150), Size);
}

ObjXOR::ObjXOR ()
{
	TypeId = O_XOR;
	_vertices = MakeOctogon(sf::Color(250, 150, 250), Size);
}

ObjNOT::ObjNOT ()
{
	TypeId = O_NOT;
	_vertices = MakeSmallSquare(sf::Color(250, 50, 50), Size);
}

ObjSwitch::ObjSwitch ()
{
	TypeId = O_Switch;
	_vertices = MakeOctogon(sf::Color(100, 50, 250), Size);
}

ObjPanel::ObjPanel ()
{
	TypeId = O_Panel;
	Size *= 2.f;
	Connectable = false;
	RemakeRect();
}

ObjRandom::ObjRandom ()
{
	TypeId = O_Random;
	_vertices = MakeOctogon(sf::Color(150, 75, 0), Size);
}

ObjBit::ObjBit ()
{
	TypeId = O_Bit;
	_vertices = MakeOctogon(sf::Color(50, 50, 250), Size);
}

ObjIndicator::ObjIndicator ()
{
	TypeId = O_Indicator;
	_vertices = MakeOctogon(sf::Color::Black, Size);
}



void ObjSwitch::Click ()
{
	IsOn = !IsOn;
	StageElectrified = IsOn && actOR();
}

void ObjBit::Click ()
{
	IsOn = !IsOn;
}


bool Obj::actOR ()
{
	for (auto &con : Connections)
		if (!con->IsHost && con->Foreign->Electrified) return true;
	return false;
}


bool ObjAND::getActive ()
{
	if (!Connections.size()) return false;
	bool live = false;
	for (auto &con : Connections) {
		if (con->IsHost) continue;
		if (!con->Foreign->Electrified) return false;
		else live = true;
	}
	return live;
}

bool ObjOR::getActive ()
{
	return actOR();
}

bool ObjXOR::getActive ()
{
	uint8_t live = 0;
	for (auto &con : Connections) {
		if (con->IsHost || !con->Foreign->Electrified) continue;
		++live;
		if (live > 1) return false;
	}
	return live == 1;
}

bool ObjNOT::getActive ()
{
	for (auto &con : Connections)
		if (!con->IsHost && con->Foreign->Electrified) return false;
	return true;
}

bool ObjSwitch::getActive ()
{
	bool beSource = true;
	//If we have any input connections, don't be a source
	for (auto &con : Connections)
		if (!con->IsHost) beSource = false;
	return IsOn ? (beSource ? true : actOR()) : false;
}

bool ObjPanel::getActive ()
{
	return false;
}

bool ObjRandom::getActive ()
{
	return rand() % 2;
}

bool ObjBit::getActive ()
{
	uint8_t numLive = 0;
	for (auto &con : Connections)
		if (!con->IsHost && con->Foreign->Electrified) ++numLive;
	if (numLive >= 2) IsOn = true; 
	if (numLive == 1) IsOn = false;
	return IsOn;
}

bool ObjIndicator::getActive ()
{
	bool isActive = actOR();
	if (prevActive != isActive) Recolor(isActive ? sf::Color(0, 150, 50) : sf::Color::Black);
	prevActive = isActive;
	return isActive;
}




void ObjPanel::RemakeRect () {
	delete _vertices;
	_vertices = MakeResizeRectangle(sf::Color(0, 0, 0, 32), Size);
}


static bool IsPointNearLine (sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p, float dist = 1)
{
	float px = p2.x - p1.x;
	float py = p2.y - p1.y;
	float u = ((p.x-p1.x)*px + (p.y-p1.y)*py) / (px*px + py*py);
	u = u > 1 ? 1 : (u < 0 ? 0 : u);
	float x = p1.x + (u*px);
	float y = p1.y + (u*py);
	float dx = x - p.x;
	float dy = y - p.y;
	return sqrt(dx*dx + dy*dy) < dist;
}

bool ObjX::ContainsPoint (sf::Vector2f point)
{
	return IsPointNearLine(Host->CenterPos, Foreign->CenterPos, point);
}




sf::VertexArray* Obj::MakeOctogon (sf::Color col, sf::Vector2f dim)
{
	auto vertices = new sf::VertexArray(sf::Quads, 12);
	float size = dim.x;
	auto oneQ = size * .25;
	auto thrQ = size * .75;
	(*vertices)[0] = sf::Vertex(sf::Vector2f(0, oneQ), col);
	(*vertices)[1] = sf::Vertex(sf::Vector2f(oneQ, 0), col);
	(*vertices)[2] = sf::Vertex(sf::Vector2f(oneQ, size), col),
	(*vertices)[3] = sf::Vertex(sf::Vector2f(0, thrQ), col);
	
	(*vertices)[4] = sf::Vertex(sf::Vector2f(oneQ, 0), col);
	(*vertices)[5] = sf::Vertex(sf::Vector2f(thrQ, 0), col);
	(*vertices)[6] = sf::Vertex(sf::Vector2f(thrQ, size), col),
	(*vertices)[7] = sf::Vertex(sf::Vector2f(oneQ, size), col);
	
	(*vertices)[8] = sf::Vertex(sf::Vector2f(thrQ, 0), col);
	(*vertices)[9] = sf::Vertex(sf::Vector2f(size, oneQ), col);
	(*vertices)[10]= sf::Vertex(sf::Vector2f(size, thrQ), col),
	(*vertices)[11]= sf::Vertex(sf::Vector2f(thrQ, size), col);
	RoughOffset = sf::Vector2f(0, 0);
	RoughSize = dim;
	return vertices;
}

sf::VertexArray* Obj::MakeSmallDiamond (sf::Color col, sf::Vector2f dim)
{
	auto vertices = new sf::VertexArray(sf::Quads, 12);
	float size = dim.x;
	auto oneQ = size * .25;
	auto twoQ = size * .5;
	auto thrQ = size * .75;
	(*vertices)[0] = sf::Vertex(sf::Vector2f(twoQ, oneQ), col);
	(*vertices)[1] = sf::Vertex(sf::Vector2f(thrQ, twoQ), col);
	(*vertices)[2] = sf::Vertex(sf::Vector2f(twoQ, thrQ), col),
	(*vertices)[3] = sf::Vertex(sf::Vector2f(oneQ, twoQ), col);
	RoughOffset = sf::Vector2f(oneQ, oneQ);
	RoughSize = sf::Vector2f(twoQ, twoQ);
	return vertices;
}

sf::VertexArray* Obj::MakeSmallSquare (sf::Color col, sf::Vector2f dim)
{
	auto vertices = new sf::VertexArray(sf::Quads, 12);
	float size = dim.x;
	auto oneQ = size * .25;
	auto twoQ = size * .5;
	auto thrQ = size * .75;
	(*vertices)[0] = sf::Vertex(sf::Vector2f(oneQ, oneQ), col);
	(*vertices)[1] = sf::Vertex(sf::Vector2f(thrQ, oneQ), col);
	(*vertices)[2] = sf::Vertex(sf::Vector2f(thrQ, thrQ), col),
	(*vertices)[3] = sf::Vertex(sf::Vector2f(oneQ, thrQ), col);
	RoughOffset = sf::Vector2f(oneQ, oneQ);
	RoughSize = sf::Vector2f(twoQ, twoQ);
	return vertices;
}

sf::VertexArray* Obj::MakeResizeRectangle (sf::Color col, sf::Vector2f dim)
{
	auto vertices = new sf::VertexArray(sf::Quads, 8);
	(*vertices)[0] = sf::Vertex(sf::Vector2f(0, 0), col);
	(*vertices)[1] = sf::Vertex(sf::Vector2f(dim.x, 0), col);
	(*vertices)[2] = sf::Vertex(sf::Vector2f(dim.x, dim.y), col),
	(*vertices)[3] = sf::Vertex(sf::Vector2f(0, dim.y), col);
	
	(*vertices)[4] = sf::Vertex(sf::Vector2f(dim.x-1, dim.y-1), col);
	(*vertices)[5] = sf::Vertex(sf::Vector2f(dim.x, dim.y-1), col);
	(*vertices)[6] = sf::Vertex(sf::Vector2f(dim.x, dim.y), col),
	(*vertices)[7] = sf::Vertex(sf::Vector2f(dim.x-1, dim.y), col);
	RoughOffset = sf::Vector2f(0, 0);
	RoughSize = dim;
	return vertices;
}
