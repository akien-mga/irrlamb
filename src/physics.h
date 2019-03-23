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
#pragma once
#include <ode/common.h>
#include <glm/vec3.hpp>

// Classes
class _Physics {

	public:

		enum FilterTypes {
			FILTER_NONE			= 0x0,
			FILTER_RIGIDBODY	= 0x1,
			FILTER_STATIC		= 0x2,
			FILTER_KINEMATIC	= 0x4,
			FILTER_CAMERA		= 0x8,
			FILTER_ZONE			= 0x10,
		};

		int Init();
		int Close();

		void Update(float FrameTime);
		void Reset();

		bool RaycastWorld(const glm::vec3 &Start, glm::vec3 &End);

		dWorldID GetWorld() { return World; }
		dJointGroupID GetContactGroup() { return ContactGroup; }
		dSpaceID GetSpace() { return Space; }

		void SetEnabled(bool Value) { Enabled = Value; }
		bool IsEnabled() const { return Enabled; }
		void RemoveFilter(int &Value, int Filter);

		void QuaternionToEuler(const float *Quat, float *Euler);

	private:

		dWorldID World;
		dJointGroupID ContactGroup;
		dSpaceID Space;

		bool Enabled;
};

// Singletons
extern _Physics Physics;
