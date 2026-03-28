/* BankPanel.h
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

#include <string>

class PlayerInfo;



// This is an overlay drawn on top of the PlanetPanel when the player clicks on
// the "bank" button. It shows the player's mortgages and other expenses, and
// allows them to apply for new mortgages or pay extra on existing debts.
class BankPanel : public Panel {
public:
	explicit BankPanel(PlayerInfo &player);

	virtual void Step() override;
	virtual void Draw() override;


protected:
	// Overrides from Panel.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, MouseButton button, int clicks) override;


private:
	// Callback for the dialogs asking you to enter an amount to pay extra on an
	// existing loan or the total amount for a new loan.
	void PayExtra(const std::string &str);
	void NewMortgage(const std::string &str);


private:
	PlayerInfo &player;
	// Loan amount you're prequalified for.
	int64_t qualify;
	int selectedRow = 0;

	bool mergedMortgages = false;
	int mortgageRows = 0;
};
