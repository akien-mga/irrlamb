/******************************************************************************
* irrlamb - https://github.com/jazztickets/irrlamb
* Copyright (C) 2019  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <level.h>
#include <globals.h>
#include <framework.h>
#include <objectmanager.h>
#include <scripting.h>
#include <graphics.h>
#include <save.h>
#include <replay.h>
#include <log.h>
#include <physics.h>
#include <input.h>
#include <audio.h>
#include <config.h>
#include <objects/template.h>
#include <objects/player.h>
#include <objects/plane.h>
#include <objects/sphere.h>
#include <objects/box.h>
#include <objects/orb.h>
#include <objects/cylinder.h>
#include <objects/terrain.h>
#include <objects/zone.h>
#include <objects/trimesh.h>
#include <objects/constraint.h>
#include <tinyxml2/tinyxml2.h>
#include <ISceneManager.h>
#include <IFileSystem.h>

_Level Level;

using namespace irr;
using namespace tinyxml2;

// Handle user data from .irr file
void _UserDataLoader::OnReadUserData(irr::scene::ISceneNode *ForSceneNode, irr::io::IAttributes *UserData) {

	for(uint32_t i = 0; i < UserData->getAttributeCount(); i++) {
		core::stringc Name(UserData->getAttributeName(i));

		if(Name == "BackgroundColor") {
			video::SColorf FloatColor(UserData->getAttributeAsColorf(i));
			Level.ClearColor.set(255, (uint32_t)(255 * FloatColor.r), (uint32_t)(255 * FloatColor.g), (uint32_t)(255 * FloatColor.b));
		}
	}

	return;
}

// Loads a level file
int _Level::Init(const std::string &LevelName, bool HeaderOnly) {
	if(!HeaderOnly)
		Log.Write("Loading level: %s", LevelName.c_str());

	// Get fastest time
	FastestTime = 0.0f;
	const _LevelStat &Stats = Save.LevelStats[LevelName];
	if(Stats.HighScores.size() > 0)
		FastestTime = Stats.HighScores[0].Time;

	// Get paths
	this->LevelName = LevelName;
	LevelNiceName = "";
	std::string LevelFile = LevelName + "/" + LevelName + ".xml";
	std::string FilePath = Framework.GetWorkingPath() + std::string("levels/") + LevelFile;
	std::string CustomFilePath = Save.CustomLevelsPath + LevelFile;
	CustomDataPath = Framework.GetWorkingPath() + std::string("levels/") + LevelName + "/";

	// See if custom level exists first
	IsCustomLevel = false;
	std::ifstream CustomLevelExists(CustomFilePath.c_str());
	if(CustomLevelExists) {
		IsCustomLevel = true;
		FilePath = CustomFilePath;
		CustomDataPath = Save.CustomLevelsPath + LevelName + "/";
	}
	CustomLevelExists.close();

	// Open the XML file
	XMLDocument Document;
	if(Document.LoadFile(FilePath.c_str()) != XML_SUCCESS) {
		Log.Write("Error loading level file with error id = %d", Document.ErrorID());
		Log.Write("Error string: %s", Document.ErrorStr());
		Close();
		return 0;
	}

	// Check for level tag
	XMLElement *LevelElement = Document.FirstChildElement("level");
	if(!LevelElement) {
		Log.Write("Could not find level tag");
		Close();
		return 0;
	}

	// Level version
	if(LevelElement->QueryIntAttribute("version", &LevelVersion) == XML_NO_ATTRIBUTE) {
		Log.Write("Could not find level version");
		Close();
		return 0;
	}

	// Check required game version
	GameVersion = LevelElement->Attribute("gameversion");
	if(GameVersion == "") {
		Log.Write("Could not find game version attribute");
		Close();
		return 0;
	}

	// Load level info
	XMLElement *InfoElement = LevelElement->FirstChildElement("info");
	if(InfoElement) {
		XMLElement *NiceNameElement = InfoElement->FirstChildElement("name");
		if(NiceNameElement) {
			LevelNiceName = NiceNameElement->GetText();
		}
	}

	// Return after header is read
	if(HeaderOnly) {
		Close();
		return true;
	}

	// Load default lua script
	Scripts.clear();
	Scripts.push_back(Framework.GetWorkingPath() + "scripts/default.lua");

	// Options
	bool Fog = false;
	bool EmitLight = false;
	Level.ClearColor.set(255, 0, 0, 0);
	irrScene->setAmbientLight(video::SColorf(0.3, 0.3, 0.3, 1));
	irrDriver->setFog(video::SColor(0), irr::video::EFT_FOG_EXP, 0, 0, 0);

	// Load options
	XMLElement *OptionsElement = LevelElement->FirstChildElement("options");
	if(OptionsElement) {

		// Use lights from world/player
		XMLElement *EmitLightElement = OptionsElement->FirstChildElement("emitlight");
		if(EmitLightElement) {
			EmitLightElement->QueryBoolAttribute("enabled", &EmitLight);
		}
	}

	// Load world
	XMLElement *ResourcesElement = LevelElement->FirstChildElement("resources");
	if(ResourcesElement) {

		// Load scenes
		for(XMLElement *SceneElement = ResourcesElement->FirstChildElement("scene"); SceneElement != 0; SceneElement = SceneElement->NextSiblingElement("scene")) {

			// Get file
			std::string File = SceneElement->Attribute("file");
			if(File == "") {
				Log.Write("Could not find file attribute on scene");
				Close();
				return 0;
			}

			// Reset fog
			irrDriver->setFog(video::SColor(0, 0, 0, 0), video::EFT_FOG_EXP, 0, 0, 0.0f);

			// Load scene
			if(IsCustomLevel) {
				irrFile->changeWorkingDirectoryTo(CustomDataPath.c_str());
				irrScene->loadScene(File.c_str(), &UserDataLoader);
				irrFile->changeWorkingDirectoryTo(Framework.GetWorkingPath().c_str());
			}
			else {
				irrScene->loadScene((CustomDataPath + File).c_str(), &UserDataLoader);
			}

			// Set texture filters on meshes in the scene
			core::array<irr::scene::ISceneNode *> MeshNodes;
			irrScene->getSceneNodesFromType(scene::ESNT_MESH, MeshNodes);
			for(uint32_t i = 0; i < MeshNodes.size(); i++) {
				if(EmitLight && Config.Shaders) {
					video::SMaterial &Material = MeshNodes[i]->getMaterial(0);
					int ShaderType = 0;
					if(Material.MaterialType == video::EMT_TRANSPARENT_ALPHA_CHANNEL) {
						ShaderType = 1;
					}
					MeshNodes[i]->setMaterialType((video::E_MATERIAL_TYPE)Graphics.GetCustomMaterial(ShaderType));
				}

				//MeshNodes[i]->setMaterialFlag(video::EMF_WIREFRAME, true);
				MeshNodes[i]->setMaterialFlag(video::EMF_TRILINEAR_FILTER, Config.TrilinearFiltering);
				for(uint32_t j = 0; j < MeshNodes[i]->getMaterialCount(); j++) {
					for(int k = 0; k < 4; k++) {
						MeshNodes[i]->getMaterial(j).TextureLayer[k].AnisotropicFilter = Config.AnisotropicFiltering;
						if(MeshNodes[i]->getMaterial(j).FogEnable)
							Fog = true;
					}
				}
			}
		}

		// Load collision
		for(XMLElement *CollisionElement = ResourcesElement->FirstChildElement("collision"); CollisionElement != 0; CollisionElement = CollisionElement->NextSiblingElement("collision")) {

			// Get file
			std::string File = CollisionElement->Attribute("file");
			if(File == "") {
				Log.Write("Could not find file attribute on collision");
				Close();
				return 0;
			}

			// Create template
			_Template *Template = new _Template;
			Template->CollisionFile = CustomDataPath + File;
			Template->Type = _Object::COLLISION;
			Template->CollisionGroup = _Physics::FILTER_STATIC | _Physics::FILTER_CAMERA;
			Template->CollisionMask = _Physics::FILTER_RIGIDBODY;
			Template->Mass = 0.0f;
			Templates.push_back(Template);

			// Get collision friction
			CollisionElement->QueryFloatAttribute("friction", &Template->Friction);

			// Create spawn
			_ObjectSpawn *ObjectSpawn = new _ObjectSpawn;
			ObjectSpawn->Template = Template;
			const char *AttributeName;
			if((AttributeName = CollisionElement->Attribute("name")))
				ObjectSpawn->Name = AttributeName;
			ObjectSpawns.push_back(ObjectSpawn);
		}

		// Load scripts
		for(XMLElement *ScriptElement = ResourcesElement->FirstChildElement("script"); ScriptElement != 0; ScriptElement = ScriptElement->NextSiblingElement("script")) {

			// Get file
			std::string File = ScriptElement->Attribute("file");
			if(File == "") {
				Log.Write("Could not find file attribute on script");
				Close();
				return 0;
			}

			Scripts.push_back(CustomDataPath + File);
		}

		// Load sounds
		Sounds.clear();
		for(XMLElement *SoundElement = ResourcesElement->FirstChildElement("sound"); SoundElement != 0; SoundElement = SoundElement->NextSiblingElement("sound")) {

			// Get file
			std::string File = SoundElement->Attribute("file");
			if(File == "") {
				Log.Write("Could not find file attribute on sound");
				Close();
				return 0;
			}

			// Attempt to load sound
			if(Audio.LoadBuffer(File))
				Sounds.push_back(File);
		}
	}

	// Load templates
	XMLElement *TemplatesElement = LevelElement->FirstChildElement("templates");
	int TemplateID = 0;
	if(TemplatesElement) {
		for(XMLElement *TemplateElement = TemplatesElement->FirstChildElement(); TemplateElement != 0; TemplateElement = TemplateElement->NextSiblingElement()) {

			// Create a template
			_Template *Template = new _Template;
			Template->Fog = Fog;

			// Get the template properties
			if(!GetTemplateProperties(TemplateElement, *Template))
				return 0;

			// Assign options
			Template->TemplateID = TemplateID;
			if(EmitLight) {

				// Use shaders on materials that receive light
				if(Config.Shaders)
					Template->CustomMaterial = Graphics.GetCustomMaterial(0);

				// Set the player to emit light
				if(Template->Type == _Object::PLAYER || Template->Type == _Object::ORB)
					Template->EmitLight = true;
			}
			TemplateID++;

			// Store for later
			Templates.push_back(Template);
		}
	}

	// Create zones from 'empty' node types
	core::array<irr::scene::ISceneNode *> MeshNodes;
	irrScene->getSceneNodesFromType(scene::ESNT_EMPTY, MeshNodes);
	for(uint32_t i = 0; i < MeshNodes.size(); i++) {

		// Create zone template
		_Template *Template = new _Template;
		Template->Name = MeshNodes[i]->getName();
		Template->TemplateID = TemplateID++;
		Template->Type = _Object::ZONE;
		Template->Mass = 0.0f;
		Template->CollisionGroup = _Physics::FILTER_ZONE;
		Template->CollisionMask = _Physics::FILTER_RIGIDBODY;
		Template->Shape[0] = std::abs(MeshNodes[i]->getScale().X * 2);
		Template->Shape[1] = std::abs(MeshNodes[i]->getScale().Y * 2);
		Template->Shape[2] = std::abs(MeshNodes[i]->getScale().Z * 2);
		Templates.push_back(Template);

		// Create object spawn
		_ObjectSpawn *ObjectSpawn = new _ObjectSpawn;
		ObjectSpawn->Name = MeshNodes[i]->getName();
		ObjectSpawn->Position[0] = MeshNodes[i]->getPosition().X;
		ObjectSpawn->Position[1] = MeshNodes[i]->getPosition().Y;
		ObjectSpawn->Position[2] = MeshNodes[i]->getPosition().Z;
		ObjectSpawn->Template = Template;
		ObjectSpawns.push_back(ObjectSpawn);
	}

	// Load object spawns
	XMLElement *ObjectsElement = LevelElement->FirstChildElement("objects");
	if(ObjectsElement) {
		for(XMLElement *ObjectElement = ObjectsElement->FirstChildElement(); ObjectElement != 0; ObjectElement = ObjectElement->NextSiblingElement()) {

			// Create an object spawn
			_ObjectSpawn *ObjectSpawn = new _ObjectSpawn;

			// Get the object properties
			if(!GetObjectSpawnProperties(ObjectElement, *ObjectSpawn))
				return 0;

			// Store for later
			ObjectSpawns.push_back(ObjectSpawn);
		}
	}

	// Load constraint spawns
	XMLElement *ConstraintsElement = LevelElement->FirstChildElement("constraints");
	if(ConstraintsElement) {
		for(XMLElement *ConstraintElement = ConstraintsElement->FirstChildElement(); ConstraintElement != 0; ConstraintElement = ConstraintElement->NextSiblingElement()) {

			// Create a constraint spawn
			_ConstraintSpawn *ConstraintSpawn = new _ConstraintSpawn;

			// Get the constraint properties
			if(!GetConstraintSpawnProperties(ConstraintElement, *ConstraintSpawn))
				return 0;

			// Store for later
			ConstraintSpawns.push_back(ConstraintSpawn);
		}
	}

	return 1;
}

// Closes the level
int _Level::Close() {

	// Clear scripts
	Scripts.clear();

	// Clear sounds
	for(size_t i = 0; i < Sounds.size(); i++)
		Audio.CloseBuffer(Sounds[i]);

	Sounds.clear();

	// Delete templates
	for(size_t i = 0; i < Templates.size(); i++)
		delete Templates[i];

	Templates.clear();

	// Delete object spawn data
	for(size_t i = 0; i < ObjectSpawns.size(); i++)
		delete ObjectSpawns[i];

	ObjectSpawns.clear();

	// Delete constraint spawn data
	for(size_t i = 0; i < ConstraintSpawns.size(); i++)
		delete ConstraintSpawns[i];

	ConstraintSpawns.clear();

	return 1;
}

// Processes a template tag
int _Level::GetTemplateProperties(XMLElement *TemplateElement, _Template &Template) {
	XMLElement *Element;
	const char *String;

	// Get type
	std::string ObjectType = TemplateElement->Value();

	// Object defaults
	if(ObjectType == "orb") {
		Template.Sleep = true;
	}

	// Get name
	String = TemplateElement->Attribute("name");
	if(!String) {
		Log.Write("Template is missing name");
		return 0;
	}
	Template.Name = String;

	// Get attributes
	TemplateElement->QueryFloatAttribute("lifetime", &Template.Lifetime);
	TemplateElement->QueryIntAttribute("smooth", &Template.Smooth);
	TemplateElement->QueryIntAttribute("detail", &Template.Detail);

	// Get scale
	Element = TemplateElement->FirstChildElement("shape");
	if(Element) {
		Element->QueryFloatAttribute("w", &Template.Shape[0]);
		Element->QueryFloatAttribute("h", &Template.Shape[1]);
		Element->QueryFloatAttribute("l", &Template.Shape[2]);
		Element->QueryFloatAttribute("r", &Template.Radius);
	}

	// Get mesh properties
	Element = TemplateElement->FirstChildElement("mesh");
	if(Element) {
		String = Element->Attribute("file");
		if(String) {
			Template.Mesh = String;

			// Try custom path first
			Template.Mesh = CustomDataPath + std::string("meshes/") + String;
			if(!irrFile->existFile(Template.Mesh.c_str())) {

				// Try normal path
				Template.Mesh = Framework.GetWorkingPath() + std::string("meshes/") + String;
				if(!irrFile->existFile(Template.Mesh.c_str())) {
					Log.Write("Mesh file does not exist: %s", String);
					return 0;
				}
			}
		}

		// Get component scale
		Element->QueryFloatAttribute("w", &Template.Scale[0]);
		Element->QueryFloatAttribute("h", &Template.Scale[1]);
		Element->QueryFloatAttribute("l", &Template.Scale[2]);

		// Get scale
		float Scale = 1.0f;
		Element->QueryFloatAttribute("scale", &Scale);
		for(int i = 0; i < 3; i++)
			Template.Scale[i] *= Scale;
	}

	// Get heightmap properties
	Element = TemplateElement->FirstChildElement("heightmap");
	if(Element) {
		String = Element->Attribute("file");
		if(String) {
			Template.HeightMap = String;

			// Try custom path first
			Template.HeightMap = CustomDataPath + std::string("textures/") + String;
			if(!irrFile->existFile(Template.HeightMap.c_str())) {

				// Try normal path
				Template.HeightMap = Framework.GetWorkingPath() + std::string("textures/") + String;
				if(!irrFile->existFile(Template.HeightMap.c_str())) {
					Log.Write("Heightmap file does not exist: %s", String);
					return 0;
				}
			}
		}
	}

	// Get physical attributes
	Element = TemplateElement->FirstChildElement("physics");
	if(Element) {
		Element->QueryIntAttribute("kinematic", &Template.Kinematic);
		Element->QueryIntAttribute("sleep", &Template.Sleep);
		Element->QueryFloatAttribute("mass", &Template.Mass);
		Element->QueryFloatAttribute("friction", &Template.Friction);
		Element->QueryFloatAttribute("rolling_friction", &Template.RollingFriction);
		Element->QueryFloatAttribute("restitution", &Template.Restitution);
		Element->QueryFloatAttribute("erp", &Template.ERP);
		Element->QueryFloatAttribute("cfm", &Template.CFM);
	}

	// Get collision attributes
	Element = TemplateElement->FirstChildElement("collision");
	if(Element) {
		Element->QueryIntAttribute("group", &Template.CollisionGroup);
		Element->QueryIntAttribute("mask", &Template.CollisionMask);
		String = Element->Attribute("callback");
		if(String)
			Template.CollisionCallback = String;
	}

	// Get damping
	Element = TemplateElement->FirstChildElement("damping");
	if(Element) {
		Element->QueryFloatAttribute("linear", &Template.LinearDamping);
		Element->QueryFloatAttribute("angular", &Template.AngularDamping);
	}

	// Get axis for constraints
	Element = TemplateElement->FirstChildElement("axis");
	if(Element) {
		Element->QueryFloatAttribute("x", &Template.ConstraintAxis[0]);
		Element->QueryFloatAttribute("y", &Template.ConstraintAxis[1]);
		Element->QueryFloatAttribute("z", &Template.ConstraintAxis[2]);
	}

	// Get textures
	for(XMLElement *Element = TemplateElement->FirstChildElement("texture"); Element != 0; Element = Element->NextSiblingElement("texture")) {

		// Get texture index
		int Index = 0;
		Element->QueryIntAttribute("index", &Index);
		if(Index > 3) {
			Log.Write("Texture index out of bounds! %d > 3", Index);
			return 0;
		}

		// Get texture scale
		Element->QueryFloatAttribute("scale", &Template.TextureScale[Index]);

		// Get filename
		const char *Filename = Element->Attribute("file");
		if(Filename) {

			// Try custom path first
			Template.Textures[Index] = CustomDataPath + std::string("textures/") + Filename;
			if(!irrFile->existFile(Template.Textures[Index].c_str())) {

				// Try normal path
				Template.Textures[Index] = Framework.GetWorkingPath() + std::string("textures/") + Filename;
				if(!irrFile->existFile(Template.Textures[Index].c_str())) {
					Log.Write("Texture file does not exist: %s", Filename);
					return 0;
				}
			}
		}
	}

	// Validate objects
	if(ObjectType == "player") {
		Template.Type = _Object::PLAYER;
		Template.RollingFriction = 0.001f;
		Template.CollisionGroup &= ~_Physics::FILTER_CAMERA;
		Template.Fog = false;
	}
	else if(ObjectType == "orb") {
		Template.Type = _Object::ORB;
	}
	else if(ObjectType == "plane") {
		Template.Type = _Object::PLANE;
		Template.Mass = 0.0f;
	}
	else if(ObjectType == "sphere") {
		Template.Type = _Object::SPHERE;
	}
	else if(ObjectType == "box") {
		Template.Type = _Object::BOX;
	}
	else if(ObjectType == "cylinder") {
		Template.Type = _Object::CYLINDER;
	}
	else if(ObjectType == "terrain") {
		Template.Type = _Object::TERRAIN;
		Template.Mass = 0.0f;
	}
	else if(ObjectType == "zone") {
		Template.Type = _Object::ZONE;
		Template.Mass = 0.0f;
		Template.CollisionGroup = _Physics::FILTER_ZONE;
		Template.CollisionMask = _Physics::FILTER_RIGIDBODY;
	}
	else if(ObjectType == "d6") {
		Template.Type = _Object::CONSTRAINT_D6;
	}
	else if(ObjectType == "fixed") {
		Template.Type = _Object::CONSTRAINT_FIXED;
	}
	else if(ObjectType == "hinge") {
		Template.Type = _Object::CONSTRAINT_HINGE;
	}

	// If a body's mass is zero, set group to static
	if(Template.Mass == 0.0f && (Template.CollisionGroup & _Physics::FILTER_RIGIDBODY)) {
		Template.CollisionGroup = _Physics::FILTER_STATIC | _Physics::FILTER_CAMERA;
		Template.CollisionMask = 0;
	}

	return 1;
}

// Processes an object tag
int _Level::GetObjectSpawnProperties(XMLElement *ObjectElement, _ObjectSpawn &ObjectSpawn) {
	XMLElement *Element;

	// Get name
	ObjectSpawn.Name = ObjectElement->Attribute("name");
	if(ObjectSpawn.Name == "") {
		Log.Write("Object is missing name");
		return 0;
	}

	// Get template name
	std::string TemplateName = ObjectElement->Attribute("template");
	if(TemplateName == "") {
		Log.Write("Object is missing template name");
		return 0;
	}

	// Get template data
	ObjectSpawn.Template = GetTemplate(TemplateName);
	if(ObjectSpawn.Template == nullptr) {
		Log.Write("Cannot find object template %s", TemplateName.c_str());
		return 0;
	}

	// Get position
	Element = ObjectElement->FirstChildElement("position");
	if(Element) {
		Element->QueryFloatAttribute("x", &ObjectSpawn.Position[0]);
		Element->QueryFloatAttribute("y", &ObjectSpawn.Position[1]);
		Element->QueryFloatAttribute("z", &ObjectSpawn.Position[2]);
	}

	// Get euler rotation
	Element = ObjectElement->FirstChildElement("rotation");
	if(Element) {
		Element->QueryFloatAttribute("x", &ObjectSpawn.Rotation[0]);
		Element->QueryFloatAttribute("y", &ObjectSpawn.Rotation[1]);
		Element->QueryFloatAttribute("z", &ObjectSpawn.Rotation[2]);
	}

	// Get quaternion rotation
	Element = ObjectElement->FirstChildElement("quaternion");
	if(Element) {
		Element->QueryFloatAttribute("x", &ObjectSpawn.Quaternion.x);
		Element->QueryFloatAttribute("y", &ObjectSpawn.Quaternion.y);
		Element->QueryFloatAttribute("z", &ObjectSpawn.Quaternion.z);
		Element->QueryFloatAttribute("w", &ObjectSpawn.Quaternion.w);
		ObjectSpawn.HasQuaternion = true;
		ObjectSpawn.CalculateRotation();
	}

	// Get plane
	Element = ObjectElement->FirstChildElement("plane");
	if(Element) {

		// Get normal
		glm::vec3 Normal;
		Element->QueryFloatAttribute("x", &Normal[0]);
		Element->QueryFloatAttribute("y", &Normal[1]);
		Element->QueryFloatAttribute("z", &Normal[2]);
		Element->QueryFloatAttribute("d", &ObjectSpawn.Plane[3]);

		// Normalize
		if(Normal != glm::vec3(0))
			Normal = glm::normalize(Normal);

		// Set plane
		ObjectSpawn.Plane = glm::vec4(Normal, ObjectSpawn.Plane[3]);
	}

	// Get linear velocity
	Element = ObjectElement->FirstChildElement("linear_velocity");
	if(Element) {
		Element->QueryFloatAttribute("x", &ObjectSpawn.LinearVelocity[0]);
		Element->QueryFloatAttribute("y", &ObjectSpawn.LinearVelocity[1]);
		Element->QueryFloatAttribute("z", &ObjectSpawn.LinearVelocity[2]);
	}

	// Get angular velocity
	Element = ObjectElement->FirstChildElement("angular_velocity");
	if(Element) {
		Element->QueryFloatAttribute("x", &ObjectSpawn.AngularVelocity[0]);
		Element->QueryFloatAttribute("y", &ObjectSpawn.AngularVelocity[1]);
		Element->QueryFloatAttribute("z", &ObjectSpawn.AngularVelocity[2]);
	}

	return 1;
}

// Processes a constraint tag
int _Level::GetConstraintSpawnProperties(XMLElement *ConstraintElement, _ConstraintSpawn &ConstraintSpawn) {
	XMLElement *Element;
	const char *String = nullptr;

	// Get name
	ConstraintSpawn.Name = ConstraintElement->Attribute("name");
	if(ConstraintSpawn.Name == "") {
		Log.Write("Constraint is missing name");
		return 0;
	}

	// Get template name
	std::string TemplateName = ConstraintElement->Attribute("template");
	if(TemplateName == "") {
		Log.Write("Constraint is missing template name");
		return 0;
	}

	// Get template data
	ConstraintSpawn.Template = GetTemplate(TemplateName);
	if(ConstraintSpawn.Template == nullptr) {
		Log.Write("Cannot find constraint template %s", TemplateName.c_str());
		return 0;
	}

	// Get first object
	String = ConstraintElement->Attribute("object1");
	if(String)
		ConstraintSpawn.MainObjectName = String;

	// Requirements
	if(ConstraintSpawn.MainObjectName == "") {
		Log.Write("Constraint is missing object1's name");
		return 0;
	}

	// Get second object
	String = ConstraintElement->Attribute("object2");
	if(String)
		ConstraintSpawn.OtherObjectName = String;

	// Get position
	Element = ConstraintElement->FirstChildElement("anchor_position");
	if(Element) {
		Element->QueryFloatAttribute("x", &ConstraintSpawn.AnchorPosition[0]);
		Element->QueryFloatAttribute("y", &ConstraintSpawn.AnchorPosition[1]);
		Element->QueryFloatAttribute("z", &ConstraintSpawn.AnchorPosition[2]);
		ConstraintSpawn.HasAnchorPosition = true;
	}

	return 1;
}

// Spawns all of the objects and constraints in the level
void _Level::SpawnEntities() {

	// Create objects
	for(size_t i = 0; i < ObjectSpawns.size(); i++) {
		_ObjectSpawn *Spawn = ObjectSpawns[i];
		CreateObject(*Spawn);
	}

	// Create constraints
	for(size_t i = 0; i < ConstraintSpawns.size(); i++) {
		_ConstraintSpawn *Spawn = ConstraintSpawns[i];
		Spawn->MainObject = ObjectManager.GetObjectByName(Spawn->MainObjectName);
		Spawn->OtherObject = ObjectManager.GetObjectByName(Spawn->OtherObjectName);
		CreateConstraint(*Spawn);
	}
}

// Creates an object from a spawn struct
_Object *_Level::CreateObject(const _ObjectSpawn &Object) {

	// Add object
	_Object *NewObject = nullptr;
	switch(Object.Template->Type) {
		case _Object::PLAYER:
			NewObject = ObjectManager.AddObject(new _Player(Object));
		break;
		case _Object::ORB:
			NewObject = ObjectManager.AddObject(new _Orb(Object));
		break;
		case _Object::COLLISION:
			NewObject = ObjectManager.AddObject(new _Trimesh(Object));
		break;
		case _Object::PLANE:
			NewObject = ObjectManager.AddObject(new _Plane(Object));
		break;
		case _Object::SPHERE:
			NewObject = ObjectManager.AddObject(new _Sphere(Object));
		break;
		case _Object::BOX:
			NewObject = ObjectManager.AddObject(new _Box(Object));
		break;
		case _Object::CYLINDER:
			NewObject = ObjectManager.AddObject(new _Cylinder(Object));
		break;
		case _Object::TERRAIN:
			NewObject = ObjectManager.AddObject(new _Terrain(Object));
		break;
		case _Object::ZONE:
			NewObject = ObjectManager.AddObject(new _Zone(Object));
		break;
	}

	// Record replay event
	if(Replay.IsRecording() && Object.Template->TemplateID != -1) {

		// Write replay information
		std::fstream &ReplayFile = Replay.GetFile();
		Replay.WriteEvent(_Replay::PACKET_CREATE);
		ReplayFile.write((char *)&Object.Template->TemplateID, sizeof(Object.Template->TemplateID));
		ReplayFile.write((char *)&NewObject->GetID(), sizeof(NewObject->GetID()));
		if(Object.Template->Type == _Object::PLANE) {
			ReplayFile.put(1);
			ReplayFile.write((char *)&Object.Plane, sizeof(float) * 4);
		}
		else {
			ReplayFile.put(0);
			ReplayFile.write((char *)&Object.Position, sizeof(float) * 3);
		}
		ReplayFile.write((char *)&Object.Rotation, sizeof(float) * 3);
	}

	return NewObject;
}

// Creates a constraint from a template
_Object *_Level::CreateConstraint(const _ConstraintSpawn &Constraint) {

	// Add object
	_Object *NewObject = ObjectManager.AddObject(new _Constraint(Constraint));

	return NewObject;
}

// Gets a template by name
_Template *_Level::GetTemplate(const std::string &Name) {

	// Search templates by name
	for(size_t i = 0; i < Templates.size(); i++) {
		if(Templates[i]->Name == Name) {
			return Templates[i];
		}
	}

	return nullptr;
}

// Gets a template by name
_Template *_Level::GetTemplateFromID(int ID) {

	// Search templates by id
	for(size_t i = 0; i < Templates.size(); i++) {
		if(Templates[i]->TemplateID == ID) {
			return Templates[i];
		}
	}

	return nullptr;
}

// Runs the level's scripts
void _Level::RunScripts() {

	// Reset Lua state
	Scripting.Reset();

	// Add key names to lua scope
	Scripting.DefineLuaVariable("KEY_FORWARD", Input.GetKeyName(Actions.GetInputForAction(_Input::KEYBOARD, _Actions::MOVE_FORWARD)));
	Scripting.DefineLuaVariable("KEY_BACK", Input.GetKeyName(Actions.GetInputForAction(_Input::KEYBOARD, _Actions::MOVE_BACK)));
	Scripting.DefineLuaVariable("KEY_LEFT", Input.GetKeyName(Actions.GetInputForAction(_Input::KEYBOARD, _Actions::MOVE_LEFT)));
	Scripting.DefineLuaVariable("KEY_RIGHT", Input.GetKeyName(Actions.GetInputForAction(_Input::KEYBOARD, _Actions::MOVE_RIGHT)));
	Scripting.DefineLuaVariable("KEY_RESET", Input.GetKeyName(Actions.GetInputForAction(_Input::KEYBOARD, _Actions::RESET)));
	Scripting.DefineLuaVariable("KEY_JUMP", Input.GetKeyName(Actions.GetInputForAction(_Input::KEYBOARD, _Actions::JUMP)));

	// Run scripts
	for(uint32_t i = 0; i < Scripts.size(); i++) {

		// Load a level
		Scripting.LoadFile(Scripts[i]);
	}
}
