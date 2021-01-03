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

#include "Information.h"
#include "text/WrappedText.h"

class News;
class PlayerInfo;


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
	const News *PickNews() const;
	
	
private:
	PlayerInfo &player;
	WrappedText text;
	
	// Current news item (if any):
	bool hasNews = false;
	bool hasPortrait = false;
	int portraitWidth;
	int normalWidth;
	Information newsInfo;
	WrappedText newsMessage;
};



#endif
