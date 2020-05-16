/* SpaceportPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SPACEPORT_PANEL_H_
#define SPACEPORT_PANEL_H_

#include "Panel.h"
#include "News.h"

#include "Information.h"
#include "WrappedText.h"

#include <map>
#include <set>

class PlayerInfo;
class Sprite;


// GUI panel to be shown when you are in a spaceport. This just draws the port
// description, but can also pop up conversation panels or dialogs offering
// missions that are marked as originating in the spaceport.
class SpaceportPanel : public Panel {
public:
	explicit SpaceportPanel(PlayerInfo &player);
	
	void UpdateNews();
	
	virtual void Step() override;
	virtual void Draw() override;
	
	
private:
	PlayerInfo &player;
	WrappedText text;
	
	// Current news item (if any):
	bool hasNews = false;
	Information newsInfo;
	WrappedText newsMessage;
	// After displaying a portrait for a particular profession,
	// only show it for that same profession.
	std::map<const Sprite *, std::string> displayedProfessions;
	std::map<const News *, std::set<const Sprite *> > professionsByNews;
};



#endif
