// Copyright (C) 2017-2019 Alexandre-Xavier Labonté-Lamoureux
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// tick.cpp
// Single instance of a game action sampled per tick for a player
// Sometimes called a "tic command"

#include "tick.h"

#include <SDL2/SDL_endian.h>	/* SDL_BYTEORDER, SDL_BIG_ENDIAN */

#include <string>
#include <vector>

using namespace std;

Tick::Tick()
{
	Reset();
	quit = false;
	id = 0;	// Must be changed when playing multiplayer
}

void Tick::Reset()
{
	// Init to default values
	forward = 0;
	lateral = 0;
	rotation = 0;
	vertical = 0;
	fire = false;
	chat = "";
}

// Encodes a player's data to a buffer for network usage
vector<unsigned char> Tick::Serialize() const
{
	// Serialize the command
	vector<unsigned char> c;
	c.resize(8, 0);	// It must be at least 8 bytes long

	c[0] = quit;
	c[0] = c[0] << 1;

	c[0] = c[0] | fire;
	c[0] = c[0] << 6;

	c[0] = c[0] | id;

	c[1] = forward;
	c[2] = lateral;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	c[3] = rotation;
	c[4] = rotation >> 8;

	c[5] = vertical;
	c[6] = vertical >> 8;
#else
	c[3] = rotation >> 8;
	c[4] = rotation;

	c[5] = vertical >> 8;
	c[6] = vertical;
#endif

	// Save the chat string's size
	c[7] = chat.size();

	// Write chat string to buffer
	for (unsigned int i = 0; i < chat.size(); i++)
	{
		c.push_back(chat[i]);
	}

	return c;
}

// Decodes a player's data from a buffer and write to command
void Tick::Deserialize(vector<unsigned char> v)
{
	// Safety check if not a least 8 bytes
	if (v.size() < 8)
		return;

	// Deserialize the command
	quit = v[0] & 128;
	fire = v[0] & 64;
	id = v[0] & 63;

	forward = v[1];
	lateral = v[2];

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rotation = v[4];
	rotation <<= 8;
	rotation |= v[3];

	vertical = v[6];
	vertical <<= 8;
	vertical |= v[5];
#else
	rotation = v[3];
	rotation <<= 8;
	rotation |= v[4];

	vertical = v[5];
	vertical <<= 8;
	vertical |= v[6];
#endif

	// Empty the chat sting inside the command
	chat.clear();

	// Write the chat string to the command
	for (unsigned int i = 0; i < v[7] && i < 36; i++)
	{
		chat.push_back(v[i + 8]);
	}
}
