#include "Display.hpp"
#include "Object.hpp"
#include "FileIO.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <string>

#define keyPressed(key) sf::Keyboard::isKeyPressed(key)
#define WIN_W 800
#define WIN_H 600

static void Electrify (std::vector<Obj*> &objects);
static bool IsClockElapsed (sf::Clock*, uint16_t);
static void DeleteObj (Obj*, std::vector<Obj*>&);
static sf::Vector2f SnapPos (sf::Vector2f p) { return vround(p, 2.5f); }
static void MovePanel (sf::Vector2f, Obj*, std::vector<Obj*>&);
static void StartPanelDrag (Obj*, std::vector<Obj*>&, sf::Vector2f);
static void SnapPanelObjs (Obj* oPanel, std::vector<Obj*>&);
static void DuplicateObjs (std::vector<Obj*>&, sf::Vector2f);

int main ()
{
	sf::RenderWindow window (sf::VideoMode(WIN_W, WIN_H), "Sircuit", sf::Style::Default, sf::ContextSettings(0, 0, 4));
	auto display = Display(window);
	
	auto objects = std::vector<Obj*>();
	
	sf::Transform trans;
	bool isPanning = false, isDragging = false;
	Obj* mObj = NULL;
	Obj* connObj = NULL;
	sf::Vector2f prevDrag;
	sf::Vector2f prevClick;
	bool clickSpent = false;
	bool wasLeftClick = false;
	bool wasRightClick = false;
	bool paused = false;
	
	trans.translate(WIN_W/2, WIN_H/2);
	trans.scale(10, 10);

	auto clock = std::chrono::system_clock::now();
	sf::Clock powerClock;
	
	while (window.isOpen()) {

		auto mPos = sf::Vector2f(sf::Mouse::getPosition(window));
	    auto mLoc = trans.getInverse().transformPoint(mPos);

		sf::Event event;
		while (window.pollEvent(event)) {
			//Events
			if (event.type == sf::Event::Closed) window.close();
			if (event.type == sf::Event::Resized) window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
			
			//Check if adding a new Obj
			if (event.type == sf::Event::KeyPressed) {
				if (keyPressed(sf::Keyboard::LControl)) {
					switch (event.key.code) {
						case sf::Keyboard::S: SaveToFile(objects); break;
						case sf::Keyboard::C: SaveToFile(objects, true); break;
						case sf::Keyboard::O: OpenFromFile(objects); break;
						case sf::Keyboard::I: OpenFromFile(objects, true, mLoc); break;
					}
				} else {
					switch (event.key.code) {
						case sf::Keyboard::A: AddObj(objects, new ObjAND(), mLoc); break;
						case sf::Keyboard::O: AddObj(objects, new ObjOR(), mLoc); break;
						case sf::Keyboard::X: AddObj(objects, new ObjXOR(), mLoc); break;
						case sf::Keyboard::N: AddObj(objects, new ObjNOT(), mLoc); break;
						case sf::Keyboard::S: AddObj(objects, new ObjSwitch(), mLoc); break;
						case sf::Keyboard::P: AddObj(objects, new ObjPanel(), mLoc); break;
						case sf::Keyboard::R: AddObj(objects, new ObjRandom(), mLoc); break;
						case sf::Keyboard::B: AddObj(objects, new ObjBit(), mLoc); break;
						case sf::Keyboard::I: AddObj(objects, new ObjIndicator(), mLoc); break;
						case sf::Keyboard::C: DuplicateObjs(objects, mLoc); break;
						case sf::Keyboard::F1: paused = !paused; break;
						case sf::Keyboard::F2: Electrify(objects); break;
					}
				}
			}
			//Check if clicking/dragging
			bool isLeftClick = sf::Mouse::isButtonPressed(sf::Mouse::Left) || keyPressed(sf::Keyboard::LAlt);
			bool isRightClick = sf::Mouse::isButtonPressed(sf::Mouse::Right) || keyPressed(sf::Keyboard::Space);
			if (isLeftClick || isRightClick) {
			
				wasLeftClick = isLeftClick;
				wasRightClick = isRightClick;
				clickSpent = false;
				
				auto delta = prevDrag - mPos;
				//Do drag
				if (isPanning) trans.translate(-delta/trans.getMatrix()[0]);
				if (isDragging) {
					auto drag = -delta / trans.getMatrix()[0];
					if (mObj->TypeId == O_Panel) MovePanel(drag, mObj, objects);
					else mObj->move(drag);
				}
				//Begin pan/drag/click
				if (!(isPanning + isDragging)) {
					prevClick = mPos;
					mObj = NULL;
					
					//Check if mouse over object - obj preferred to be non-panel
					for (auto &obj : objects) {
						if (!obj->getRoughRect().contains(mLoc)) continue;
						if (!mObj || (mObj && obj->TypeId != O_Panel)) mObj = obj;
					}
					
					if (isLeftClick) {
						bool lShift = keyPressed(sf::Keyboard::LShift);
						if (lShift && mObj) { //Delete obj if shift pressed
							DeleteObj(mObj, objects);
							clickSpent = true;
						} else if (!mObj && !lShift) isPanning = true;
						else if (mObj) {
							isDragging = true;
							if (mObj->TypeId == O_Panel) StartPanelDrag(mObj, objects, mLoc); //If we are beginning to drag a panel, mark participant objects
						}
					}
				}
				prevDrag = mPos;
				//Cancel making a connection, if we clicked somewhere else
				if ((!mObj || !mObj->Connectable) && connObj) connObj = NULL;
			} else {
				bool wasClick = prevClick == mPos;
				
				if ((!mObj || (mObj && mObj->TypeId == O_Panel)) && wasClick && !clickSpent) {
					//Check if there was a connection under the mouse, and delete it
					for (auto &obj : objects) {
						auto conns = &obj->Connections;
						auto rIterator = remove_if(conns->begin(), conns->end(), [mLoc](ObjX* conn) { return conn->ContainsPoint(mLoc); });
						conns->erase(rIterator, conns->end());
					}
					clickSpent = true;
				} else if (mObj) { //If we had an object under the mouse
					isDragging = false;
					if (wasClick && !clickSpent) {
						if ((wasLeftClick || (connObj && wasRightClick)) && mObj->Connectable) { //Start/make a new connection?
							if (!connObj) connObj = mObj; //First connect
							else { //Second connect
								if (connObj != mObj) AddConnection(connObj, mObj);
								if (!wasRightClick) connObj = NULL;
							}
						} else if (wasRightClick) { //'Click' the object?
							connObj = NULL;
							mObj->Click();
						}
						clickSpent = true;
						clock += std::chrono::milliseconds(30); //Mitigate double click
					} else {
						//Snap to grid
						mObj->setLocation(SnapPos(mObj->getPosition()));
						if (mObj->TypeId == O_Panel) SnapPanelObjs(mObj, objects);
					}
					mObj = NULL;
				}
				isPanning = false;
			}

			if (event.type == sf::Event::MouseWheelScrolled) {
				float zoomBy = event.mouseWheelScroll.delta > 0 ? 0.9f : 1.1f;
				trans.scale(zoomBy, zoomBy, mLoc.x, mLoc.y);
			}
		}
		
		//Display
		display.Render(trans, objects, connObj, mLoc);
		//Electrify
		if (!paused && IsClockElapsed(&powerClock, 256)) Electrify(objects);
		
		//Clock
		clock += std::chrono::milliseconds(30);
		std::this_thread::sleep_until(clock);
	}
}



bool IsClockElapsed (sf::Clock* clock, uint16_t t)
{
	bool hasElapsed = clock->getElapsedTime().asMilliseconds() > t;
	if (hasElapsed) clock->restart();
	return hasElapsed;
}


void DeleteObj (Obj* delObj, std::vector<Obj*> &objs)
{
	//Remove foreign connections to this object
	for (auto &con : delObj->Connections) {
		Obj* fobj = con->Foreign;
		for (uint8_t c = 0; c < fobj->Connections.size(); ++c)
			if (fobj->Connections[c]->Foreign->Id == delObj->Id) {
				delete fobj->Connections[c];
				fobj->Connections.erase(fobj->Connections.begin() + c);
				--c;
			}
	}
	//Remove host connections
	delObj->Connections.clear();
	//Free the object
	auto rIterator = remove_if(objs.begin(), objs.end(), [&delObj](Obj* obj) { return obj == delObj; });
	delete delObj;
	objs.erase(rIterator, objs.end());
}


void StartPanelDrag (Obj* oPanel, std::vector<Obj*> &objs, sf::Vector2f mLoc)
{
	//Move panel behind all other objs by swapping its pointer with the front pointer
	auto pivot = std::find_if(objs.begin(), objs.end(), [&oPanel](Obj* o) { return o->Id == oPanel->Id; });
	if (pivot != objs.end()) std::rotate(objs.begin(), pivot, pivot + 1);
	//Check if drag or resize
	ObjPanel* panel = (ObjPanel*)oPanel;
	if (sf::FloatRect((panel->getPosition() + panel->Size) - 1, sf::Vector2f(1, 1)).contains(mLoc)) {
		panel->Resizing = true;
	} else {
		panel->Resizing = false;
		//Mark panel participants
		auto pRect = sf::FloatRect(panel->getPosition(), panel->Size);
		for (auto &obj : objs)
			obj->SelectParticipant = pRect.intersects(sf::FloatRect(obj->getPosition(), obj->Size));
	}
}

void MovePanel (sf::Vector2f by, Obj* oPanel, std::vector<Obj*> &objs)
{
	ObjPanel* panel = (ObjPanel*)oPanel;
	if (panel->Resizing) panel->Resize(panel->Size + by);
	else
		for (auto &obj : objs)
			if (obj->SelectParticipant) obj->move(by);
}

void SnapPanelObjs (Obj* oPanel, std::vector<Obj*> &objs)
{
	((ObjPanel*)oPanel)->Resize(SnapPos(oPanel->Size));
	for (auto &obj : objs)
		if (obj->SelectParticipant) obj->setLocation(SnapPos(obj->getPosition()));
}

void DuplicateObjs (std::vector<Obj*> &objs, sf::Vector2f mLoc)
{
	sf::Vector2f panelPos;
	for (auto &obj : objs)
		if (obj->SelectParticipant && obj->TypeId == O_Panel) {
			panelPos = obj->CenterPos;
			break;
		}
	//Clone objects, and collect their old and new pointers
	auto oldPointers = std::vector<Obj*>(objs.size());
	auto newPointers = std::vector<Obj*>(objs.size());
	for (auto &obj : objs) {
		if (!obj->SelectParticipant) continue;
		auto newObj = obj->Clone();
		newObj->Id = NextObjId();
		oldPointers.push_back(obj);
		newPointers.push_back(newObj);
		AddObj(objs, newObj, SnapPos(mLoc + (obj->getPosition() - panelPos)), false);
		obj->SelectParticipant = false;
	}
	//Remake old connection pointers into new ones
	for (auto &obj : objs) {
		if (!obj->SelectParticipant) continue;
		for (uint8_t c = 0; c < obj->Connections.size(); ++c) {
			auto con = &obj->Connections[c];
			int foreignFound = -1;
			for (uint16_t op = 0; op < oldPointers.size(); ++op)
				if (oldPointers[op] == (*con)->Foreign) {
					foreignFound = op;
					break;
				}
			//If foreign connection is not within the copy, delete it
			if (foreignFound == -1) obj->Connections.erase(obj->Connections.begin() + c);
			else *con = new ObjX((*con)->Host, newPointers[foreignFound], (*con)->IsHost);
		}
	}
	//Rebuild all connections by 'changing' object locations
	for (auto &obj : objs) obj->move(sf::Vector2f(0, 0));
}




void Electrify (std::vector<Obj*> &objs)
{
	//Commit staged electrifications
	for (auto &obj : objs) obj->Electrified = obj->StageElectrified;
	//Stage electification if active
	for (auto &obj : objs) obj->StageElectrified = obj->getActive();
}



