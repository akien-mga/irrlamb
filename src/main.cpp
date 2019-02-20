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
#include <framework.h>

#if defined(_WIN32) && defined(NDEBUG)

    // Remove console in windows release mode
    #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main(int ArgumentCount, char **Arguments) {

	// Initialize the game
	if(!Framework.Init(ArgumentCount, Arguments))
		return 0;

	// Main game loop
	while(!Framework.IsDone()) {

		Framework.Update();
	}

	// Shut down the system
	Framework.Close();

	return 0;
}
