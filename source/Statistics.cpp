/* Statistics.cpp
Copyright (c) 2026 by Amazinite

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Statistics.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "PilotProfile.h"
#include "PlayerInfo.h"

using namespace std;



std::filesystem::path Statistics::FilePath()
{
	return Files::Config() / "global statistics.txt";
}



Statistics::Statistics(PlayerInfo &player)
	: player(player)
{
	Load();
}



void Statistics::Load()
{
	DataFile root(FilePath());
	for(const DataNode &child : root)
	{
		const string &key = child.Token(0);
		bool hasValue = child.Size() >= 2;
		if(!hasValue)
			child.PrintTrace("Expected key to have a value:");
		else if(key == "playtime")
			playTime = child.Value(1);
		else if(key == "idle time")
			idleTime = child.Value(1);
		else
			child.PrintTrace("Skipping unrecognized attribute:");
	}
}



void Statistics::Save() const
{
	DataWriter out(FilePath());
	out.Write("playtime", playTime);
	out.Write("idle time", idleTime);
}



void Statistics::AddPlayTime(std::chrono::nanoseconds timeVal, bool updatePlayer, bool isIdle)
{
	if(isIdle)
		idleTime += timeVal.count() * .000000001;
	else
		playTime += timeVal.count() * .000000001;
	if(updatePlayer)
	{
		player.AddPlayTime(timeVal, isIdle);
		if(player.Pilot())
			player.Pilot()->AddPlayTime(timeVal, isIdle);
	}
}
