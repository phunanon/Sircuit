#include "FileIO.hpp"
#include <cstdio>
#include <iostream>
#include "Object.hpp"
#include "tinyfiledialogs.h"

static uint16_t nextObjId = 1;
uint16_t NextObjId () { return nextObjId++; }

void AddObj (std::vector<Obj*> &objs, Obj* o, sf::Vector2f pos, bool autoCenter)
{
	o->Id = NextObjId();
	o->setLocation(autoCenter ? sf::Vector2f(vround(pos - (o->Size/2.f), 2.5f)) : pos);
	objs.push_back(o);
}

void AddConnection (Obj* hostObj, Obj* foreignObj)
{
	hostObj->Connections.push_back(new ObjX(hostObj, foreignObj, true));
	foreignObj->Connections.push_back(new ObjX(foreignObj, hostObj, false));
}

void MakeIdsConsecutive (std::vector<Obj*> &objs)
{
	uint16_t newId = 1;
	for (auto &obj : objs) obj->Id = newId++;
}

void OpenFromFile (std::vector<Obj*> &objs, bool isComponent, sf::Vector2f startPos)
{
	const char* filter[1] = { "*.ss" };
	const char* openPath = tinyfd_openFileDialog("Open a design", "save.ss", 1, filter, NULL, 0);
	if (!openPath) return;
	
	FILE* rfp = fopen(openPath, "rb");
	
	uint16_t numObjs;
	fread(&numObjs, sizeof(uint16_t), 1, rfp);
	
	if (isComponent) {
		MakeIdsConsecutive(objs);
	} else {
		//Reset environment
		for (auto &obj : objs) delete obj;
		objs.clear();
		nextObjId = 1;
	}
	
	uint16_t objIdOffset = nextObjId - 1;
	
	//Load data from file
	for (uint16_t o = 0; o < numObjs; ++o) {
		uint8_t typeId;
		uint16_t objId;
		sf::Vector2f pos;
		fread(&typeId, sizeof(uint8_t), 1, rfp);
		fread(&objId, sizeof(uint16_t), 1, rfp);
		fread(&pos.x, sizeof(float), 1, rfp);
		fread(&pos.y, sizeof(float), 1, rfp);

		Obj* newObj = NULL;
		switch (typeId) {
			case 1: newObj = new ObjAND(); break;
			case 2: newObj = new ObjOR(); break;
			case 3: newObj = new ObjXOR(); break;
			case 4: newObj = new ObjNOT(); break;
			case 5: newObj = new ObjSwitch(); break;
			case 6: newObj = new ObjPanel(); break;
			case 7: newObj = new ObjRandom(); break;
			case 8: newObj = new ObjBit(); break;
			case 9: newObj = new ObjIndicator(); break;
			default: printf("Err: Unknown typeId: %d\n", typeId); break;
		}

		AddObj(objs, newObj, pos + startPos, false);

		switch (newObj->TypeId) {
			case 5:
				fread(&((ObjSwitch*)newObj)->IsOn, sizeof(bool), 1, rfp);
				break;
			case 6:
				auto newSize = sf::Vector2f();
				fread(&newSize.x, sizeof(float), 1, rfp);
				fread(&newSize.y, sizeof(float), 1, rfp);
				((ObjPanel*)newObj)->Resize(newSize);
				break;
		}
		
		uint8_t numConnections;
		fread(&numConnections, sizeof(uint8_t), 1, rfp);
		for (uint8_t c = 0; c < numConnections; ++c) {
			uint16_t foreignId;
			fread(&foreignId, sizeof(uint16_t), 1, rfp);
			if (isComponent) foreignId += objIdOffset;
			newObj->Connections.push_back(new ObjX(NULL, NULL, true, foreignId));
		}
	}

	//Replace connections' integer foreignId's with foreign pointers, and NULL hosts with actual host
	for (auto &obj : objs) {
		for (auto &con : obj->Connections) {
			if (!con->IsHost) continue;
			auto foreignId = con->ForeignId;
			if (!foreignId) continue; //Not part of a component load
			con->Host = obj;
			//Find obj based on Id
			auto foreignObj = find_if(objs.begin(), objs.end(), [&foreignId](const Obj* obj) {return obj->Id == foreignId;});
			if (foreignObj != objs.end()) {
				con->Foreign = *foreignObj;
				(*foreignObj)->Connections.push_back(new ObjX(*foreignObj, obj, false));
			} else printf("\nErr: unknown foreign Id: %d\n", foreignId);
			//Rebuild its line, now that host and foreign are no longer NULL
			con->RebuildLine();
		}
	}
}


void SaveData (const char* savePath, std::vector<Obj*> &objs, bool isComponent)
{
	MakeIdsConsecutive(objs);
	//Find left-most and top-most objects, and normalise all objs locations to them
	//Count participants
	auto cornerMost = objs[0]->getPosition();
	uint16_t numObjs = 0;
	{
		for (auto &obj : objs) {
			if (isComponent && !obj->SelectParticipant) continue;
			auto pos = obj->getPosition();
			if (pos.x < cornerMost.x) cornerMost.x = pos.x;
			if (pos.y < cornerMost.y) cornerMost.y = pos.y;
		}
		for (auto &obj : objs) {
			if (isComponent && !obj->SelectParticipant) continue;
			obj->setLocation(obj->getPosition() - cornerMost);
			++numObjs;
		}
	}
	if (numObjs == 0) {
		printf("Tried to save with nothing to save.");
		return;
	}

	FILE* wfp = fopen(savePath, "wb");
	
	fwrite(&numObjs, sizeof(uint16_t), 1, wfp);
	
	
	//Set save ID's, isolating component items
	uint16_t saveId = 1;
	for (auto &obj : objs) {
		if (isComponent && !obj->SelectParticipant) continue;
		obj->SaveId = saveId++;
	}
	
	//Save data
	for (auto &obj : objs) {
		if (isComponent && !obj->SelectParticipant) continue;
		
		auto pos = obj->getPosition();
		uint16_t numConnections = 0;
		for (auto &con : obj->Connections) numConnections += con->IsHost && (isComponent ? con->Foreign->SelectParticipant : true);
		fwrite(&obj->TypeId, sizeof(uint8_t), 1, wfp);
		fwrite(&obj->SaveId, sizeof(uint16_t), 1, wfp);
		fwrite(&pos.x, sizeof(float), 1, wfp);
		fwrite(&pos.y, sizeof(float), 1, wfp);
		switch (obj->TypeId) {
			case 5:
				fwrite(&((ObjSwitch*)obj)->IsOn, sizeof(bool), 1, wfp);
				break;
			case 6:
				fwrite(&obj->Size.x, sizeof(float), 1, wfp);
				fwrite(&obj->Size.y, sizeof(float), 1, wfp);
				break;
		}
		fwrite(&numConnections, sizeof(uint8_t), 1, wfp);
		for (auto &con : obj->Connections) {
			if (!con->IsHost) continue;
			if (isComponent && !con->Foreign->SelectParticipant) continue;
			auto foreignId = con->Foreign->SaveId;
			fwrite(&foreignId, sizeof(uint16_t), 1, wfp);
		}
	}
	
	fclose(wfp);
	
	//Restore component locations
	if (isComponent)
		for (auto &obj : objs)
			if (obj->SelectParticipant) obj->setLocation(obj->getPosition() + cornerMost);
}

void SaveToFile (std::vector<Obj*> &objs, bool isComponent)
{
	SaveData("autosave.ss", objs, isComponent);
	const char* filter[1] = { "*.ss" };
	const char* savePath = tinyfd_saveFileDialog("Save a design", "save.ss", 1, filter, NULL);
	if (!savePath) return;
	SaveData(savePath, objs, isComponent);
}
