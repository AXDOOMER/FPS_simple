// Copyright (C) 2014-2018 Alexandre-Xavier Labont�-Lamoureux
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
// physics.cpp
// What affects a thing's movement in space

#include "things.h"
#include "vecmath.h"	/* Float3 */
#include "physics.h"

#include <cmath>		/* round, isnan */
#include <limits>		/* numeric_limits<float>::lowest() */
#include <vector>
#include <utility>		/* pair */
#include <algorithm>	/* find */
using namespace std;

// Collision detection with floors
bool AdjustPlayerToFloor(Player* play, Level* lvl)
{
	float NewHeight = numeric_limits<float>::lowest();
	bool ChangeHeight = false;
	for (unsigned int i = 0; i < lvl->planes.size(); i++)
	{
		if (pointInPoly(play->PosX(), play->PosY(), lvl->planes[i]->Vertices))
		{
			float collision_point_z = PointHeightOnPoly(play->PosX(), play->PosY(), play->PosZ(), lvl->planes[i]->Vertices);
			if (!isnan(collision_point_z) && collision_point_z > NewHeight)
			{
				if (collision_point_z <= play->PosZ() + play->MaxStep)
				{
					NewHeight = collision_point_z;
					ChangeHeight = true;
				}
			}
		}
	}
	if (ChangeHeight)
	{
		if (play->PosZ() <= NewHeight + GRAVITY)
		{
			play->pos_.z = NewHeight;
			play->AirTime = 0;
			play->Jump = false;
			play->Fly = false;
		}
		else
		{
			play->pos_.z -= GRAVITY * 0.1f * play->AirTime;
			play->AirTime++;
			play->Fly = true;	// If player is falling, but has not jumped, he must not be able to jump.
		}
	}

	return ChangeHeight;
}

void ApplyGravity(Player* play)
{
	float FloorHeight = PointHeightOnPoly(play->PosX(), play->PosY(), play->PosZ(), play->plane->Vertices);
	if (!isnan(FloorHeight))
	{
		if (play->PosZ() > FloorHeight)
		{
			play->pos_.z -= GRAVITY * 0.1f * play->AirTime;
			play->AirTime++;
			play->Fly = true;	// If player is falling, but has not jumped, he must not be able to jump.
		}

		if (play->PosZ() <= FloorHeight)
		{
			play->pos_.z = FloorHeight;
			play->AirTime = 0;
			play->Jump = false;
			play->Fly = false;
		}
	}
}

vector<Plane*> FindPlanesForPlayer(Player* play, Level* lvl)
{
	vector<Plane*> planes;

	for (unsigned int i = 0; i < lvl->planes.size(); i++)
	{
		if (pointInPoly(play->PosX(), play->PosY(), lvl->planes[i]->Vertices))
		{
			planes.push_back(lvl->planes[i]);
		}
	}

	return planes;
}

Plane* GetPlaneForPlayer(Player* play, Level* lvl)
{
	vector<Plane*> planes = FindPlanesForPlayer(play, lvl);

	vector<pair<Plane*, float>> HeightOnPlanes;

	float HighestUnderPlayer = numeric_limits<float>::lowest();

	// Find the height of the player on every planes
	for (unsigned int i = 0; i < planes.size(); i++)
	{
		float Height = PointHeightOnPoly(play->PosX(), play->PosY(), play->PosZ(), planes[i]->Vertices);

		HeightOnPlanes.push_back(make_pair(planes[i], Height));

		// This results in the player being put on the highest floor
		if (Height > HighestUnderPlayer /*&& Height <= play->PosZ()*/)
		{
			HighestUnderPlayer = Height;
		}
	}

	// Return the plane where the player is
	for (unsigned int i = 0; i < HeightOnPlanes.size(); i++)
	{
		if (HeightOnPlanes[i].second == HighestUnderPlayer)
			return HeightOnPlanes[i].first;
	}

	return nullptr;	// Need to return something
}

// Travel through planes which were intersected, check if point inside.
Plane* TraceOnPolygons(Float3 origin, Float3 target, Plane* plane)
{
	vector<Plane*> tested;		// List of polys that are already checked
	tested.push_back(plane);

	for (unsigned int i = 0; i < tested[tested.size() - 1]->Neighbors.size(); i++)
	{
		Plane* p = tested[tested.size() - 1]->Neighbors[i];

		// Skip to the next adjacent plane if this one was already checked
		if (find(tested.begin(), tested.end(), p) != tested.end())
		{
			continue;
		}

		if (pointInPoly(target.x, target.y, p->Vertices))
			return p;

		// Scan each polygon edge to see if the vector is crossing
		for (unsigned int j = 0; j < p->Vertices.size(); j++)
		{
			if (CheckVectorIntersection(origin, target, p->Vertices[j], p->Vertices[ (j+1) % p->Vertices.size() ]))
			{
				tested.push_back(p);
				i = -1;		// Reset outer loop
				break;
			}
		}
	}

	return nullptr;
//	return GetPlaneForPlayer(lvl->play, lvl);
}

// Moves the player to a new position. Returns false if it can't.
bool GetPlayerToNewPosition(Float3 origin, Float3 target, float RadiusToUse, Level* lvl)
{
	if (pointInPoly(target.x, target.y, lvl->play->plane->Vertices))
	{
		// Player is in the same polygon
//		lvl->play->pos_.z = PointHeightOnPoly(target.x, target.y, target.z, lvl->play->plane->Vertices);
		return true;
	}
	else // Player has moved to another polygon
	{
		for (unsigned int i = 0; i < lvl->play->plane->Neighbors.size(); i++)
		{
			Plane* p = lvl->play->plane->Neighbors[i];

			for (unsigned int j = 0; j < p->Vertices.size(); j++)
			{
				if (CheckVectorIntersection(origin, target, p->Vertices[j], p->Vertices[(j+1) % p->Vertices.size()]))
				{
					if (!pointInPoly(target.x, target.y, p->Vertices))
					{
						//return false;		// This early return would prevent the player from climbing a stair

						// This below will find a new valid position for the player
						p = TraceOnPolygons(origin, target, lvl->play->plane);
					}

					// GetPlaneForPlayer will return 0 when the player is in the void
					if (p == nullptr)
						return false;	// Player may still get stuck, but we avoid a crash.

/*					float Height = GetPlayerHeight(lvl->play);

					if (!isnan(Height))
						lvl->play->pos_.z = Height;
					else	// Player probably walked into a wall or a stair
						return false;
*/
					lvl->play->plane = p;

					return true;
				}
			}
		}
	}

	return false;	// When "true", allows the player to move off the edge of a polygon that has no adjacing polygon
}

// Distance smaller than length (inside or touches)
bool CompareDistanceToLength(float DiffX, float DiffY, float Length)
{
	return pow(DiffX, 2) + pow(DiffY, 2) <= Length * Length;
}

// Returns true if the vector hits any walls. The vector has a circular endpoint.
bool HitsWall(Float3 origin, Float3 target, float RadiusToUse, Level* lvl)
{
	for (unsigned int i = 0; i < lvl->planes.size(); i++)
	{
		// Test vertical planes which are walls
		if (lvl->planes[i]->WallInfo != nullptr)
		{
			// Wall above or bellow player.
			if (lvl->planes[i]->WallInfo->HighZ <= origin.z + lvl->play->MaxStep || lvl->planes[i]->WallInfo->LowZ >= origin.z + 3.0f)
			{
				// Can't be any collision.
				continue;
			}

			// Create new variables for readability
			Float3 First = lvl->planes[i]->WallInfo->Vertex1;
			Float3 Second = lvl->planes[i]->WallInfo->Vertex2;

			// Get the orthogonal vector, so invert the use of 'sin' and 'cos' here.
			float OrthVectorStartX = origin.x + sin(lvl->planes[i]->WallInfo->Angle) * RadiusToUse;
			float OrthVectorStartY = origin.y + cos(lvl->planes[i]->WallInfo->Angle) * RadiusToUse;
			float OrthVectorEndX = origin.x - sin(lvl->planes[i]->WallInfo->Angle) * RadiusToUse;
			float OrthVectorEndY = origin.y - cos(lvl->planes[i]->WallInfo->Angle) * RadiusToUse;

			// Cramer's rule, inspiration taken from here: https://stackoverflow.com/a/1968345
			float WallDiffX = Second.x - First.x;    // Vector's X from (0,0)
			float WallDiffY = Second.y - First.y;    // Vector's Y from (0,0)
			float VectorWallOrthDiffX = OrthVectorEndX - OrthVectorStartX;
			float VectorWallOrthDiffY = OrthVectorEndY - OrthVectorStartY;

			float Denominator = -VectorWallOrthDiffX * WallDiffY + WallDiffX * VectorWallOrthDiffY;
			float PointWall = (-WallDiffY * (First.x - OrthVectorStartX) + WallDiffX * (First.y - OrthVectorStartY)) / Denominator;
			float PointVectorOrth = (VectorWallOrthDiffX * (First.y - OrthVectorStartY) - VectorWallOrthDiffY * (First.x - OrthVectorStartX)) / Denominator;

			// Check if a collision is detected (Not checking if touching an endpoint)
			if (PointWall >= 0 && PointWall <= 1 && PointVectorOrth >= 0 && PointVectorOrth <= 1)
			{
				// Collision detected
				return true;
			}

			// TODO: Check for a collision with both endpoints of the wall
		}
	}

	return false;
}