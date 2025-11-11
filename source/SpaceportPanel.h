/* SpaceportPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Panel.h"

#include "Information.h"
#include "text/WrappedText.h"

class Interface;
class News;
class PlayerInfo;
class Port;
class TextArea;



// GUI panel to be shown when you are in a spaceport. This just draws the port
// description, but can also pop up conversation panels or dialogs offering
// missions that are marked as originating in the spaceport.
class SpaceportPanel : public Panel {
public:
	explicit SpaceportPanel(PlayerInfo &player);

	void UpdateNews();

	virtual void Step() override;
	virtual void Draw() override;


protected:
	virtual void Resize() override;


private:
	const News *PickNews() const;


private:
	PlayerInfo &player;
	std::shared_ptr<TextArea> description;
	const Port &port;

	// Current news item (if any):
	bool hasNews = false;
	bool hasPortrait = false;
	int portraitWidth;
	int normalWidth;
	Information newsInfo;
	WrappedText newsMessage;
};
