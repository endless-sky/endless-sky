/* MessageLogPanel.h
Copyright (c) 2023 by TomGoodIdea

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MESSAGE_LOG_PANEL_H_
#define MESSAGE_LOG_PANEL_H_

#include "Panel.h"

#include "Messages.h"



// User interface panel that displays message log.
class MessageLogPanel : public Panel {
public:
	MessageLogPanel(const std::vector<std::pair<std::string, Messages::Importance>> &messages);

	// Draw this panel.
	virtual void Draw() override;


protected:
	// Event handlers.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	virtual bool Drag(double dx, double dy) override;
	virtual bool Scroll(double dx, double dy) override;


private:
	const std::vector<std::pair<std::string, Messages::Importance>> &messages;

	double width;
	// Current scroll:
	double scroll = 0.;
	double maxScroll = 0.;
};



#endif
