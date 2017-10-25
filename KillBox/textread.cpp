// textread.cpp
//Copyright (C) 2014 Alexandre-Xavier Labont�-Lamoureux
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Handles text data reading from file & writing
// Used for config files, levels and demo files.

// basic file operations
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "things.h"
#include "command.h"

using namespace std;

//Load le niveau et retourne son pointeur
Level* F_LoadLevel(string LevelName)
{
	Level* Current = new Level;

	string FileName = LevelName + ".txt";       //Added to compile in Code::Blocks,
	ifstream LevelFile(FileName.c_str());   //can't do it all on this line.
	if (LevelFile.is_open())
	{
		unsigned int Count = 0;
		string Line;
		while (!LevelFile.eof())     //Read the entire file until the end
		{
			getline(LevelFile, Line);

			if (Line.size() > 0)
			{
				Count++;
				//cout << Count << ": " << Line << '\n';

				istringstream buf(Line);
				istream_iterator<string> beg(buf), end;
				vector<string> tokens(beg, end);

				if (tokens.size() > 0 && tokens[0][0] != '#')
				{
					if (tokens[0] == "thing" && tokens.size() == 6)
					{
						cout << tokens[1] << endl;
					}
					else if (tokens[0] == "poly" && tokens.size() == 20)
					{
						cout << tokens[1] << endl;
					}
					else
					{
						cout << "INVALID LINE IGNORED AT: " << Count << endl;
					}
				}
			}
		}

		cout << "Read " << Count << " lines from file. " << endl;
		LevelFile.close();
	}
	else
	{
		delete Current;		// Erase because it's no longer useful
		Current = nullptr;
		cout << "Unable to open level info!" << endl;
	}

	return Current;	//Doit retourner un pointeur sur le niveau
}

