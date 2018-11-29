#pragma once
#include "Object.hpp"
#include "Common.hpp"

void OpenFromFile (std::vector<Obj*> &objs, bool isComponent = false, sf::Vector2f startPos = sf::Vector2f(0, 0));
void SaveToFile (std::vector<Obj*> &objs, bool isComponent = false);

uint16_t NextObjId ();
void AddObj (std::vector<Obj*> &objs, Obj* o, sf::Vector2f pos, bool autoCenter = true);
void AddConnection (Obj* hostObj, Obj* foreignObj);
