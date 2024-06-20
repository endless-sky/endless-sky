/* NameDialog.h
Copyright (c) 2024 Endless Sky contributors

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef NAME_DIALOG_H_
#define NAME_DIALOG_H_

#include "Dialog.h"

#include "text/Font.h"
#include "GameData.h"
#include "Phrase.h"
#include "Point.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <string>

using namespace std;

// The name entry dialog should include a "Random" button to choose a random
// name using the civilian ship name generator.
class NameDialog : public Dialog {
public:
	template <class T>
	NameDialog(T *panel, void (T::*fun)(const string &), const string &message)
		: Dialog(panel, fun, message) {}

	virtual void Draw() override
	{
		Dialog::Draw();

		randomPos = cancelPos - Point(80., 0.);
		SpriteShader::Draw(SpriteSet::Get("ui/dialog cancel"), randomPos);

		const Font &font = FontSet::Get(14);
		static const string label = "Random";
		Point labelPos = randomPos - .5 * Point(font.Width(label), font.Height());
		font.Draw(label, labelPos, *GameData::Colors().Get("medium"));
	}

protected:
	virtual bool Click(int x, int y, int clicks) override
	{
		Point off = Point(x, y) - randomPos;
		if(fabs(off.X()) < 40. && fabs(off.Y()) < 20.)
		{
			input = GameData::Phrases().Get("civilian")->Get();
			return true;
		}
		return Dialog::Click(x, y, clicks);
	}

private:
	Point randomPos;
};

#endif
