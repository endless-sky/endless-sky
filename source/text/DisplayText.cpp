/* DisplayText.cpp
Copyright (c) 2020 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "DisplayText.h"

using namespace std;



DisplayText::DisplayText(const char *text, Layout layout)
	: layout(layout), text(text)
{
}



DisplayText::DisplayText(const string &text, Layout layout)
	: layout(layout), text(text)
{
}



const string &DisplayText::GetText() const noexcept
{
	return text;
}



const Layout &DisplayText::GetLayout() const noexcept
{
	return layout;
}
