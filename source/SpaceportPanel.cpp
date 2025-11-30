/* SpaceportPanel.cpp
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

#include "SpaceportPanel.h"

#include "text/Alignment.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Interface.h"
#include "News.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Screen.h"
#include "TextArea.h"
#include "UI.h"

using namespace std;



SpaceportPanel::SpaceportPanel(PlayerInfo &player)
	: player(player), port(player.GetPlanet()->GetPort())
{
	SetTrapAllEvents(false);

	description = make_shared<TextArea>();
	description->SetFont(FontSet::Get(14));
	description->SetColor(*GameData::Colors().Get("bright"));
	description->SetAlignment(Alignment::JUSTIFIED);
	Resize();
	AddChild(description);

	newsMessage.SetFont(FontSet::Get(14));
}



void SpaceportPanel::UpdateNews()
{
	const News *news = PickNews();
	if(!news)
		return;
	hasNews = true;

	// Query the news interface to find out the wrap width.
	// TODO: Allow Interface to handle wrapped text directly.
	const Interface *newsUi = GameData::Interfaces().Get(Screen::Width() < 1280 ? "news (small screen)" : "news");
	portraitWidth = newsUi->GetBox("message portrait").Width();
	normalWidth = newsUi->GetBox("message").Width();

	// Randomly pick which portrait, if any, is to be shown. Depending on if
	// this news has a portrait, different interface information gets filled in.
	auto portrait = news->Portrait();
	// Cache the randomly picked results until the next update is requested.
	hasPortrait = portrait;
	newsInfo.SetSprite("portrait", portrait);
	newsInfo.SetString("name", news->SpeakerName() + ':');
	newsMessage.SetWrapWidth(hasPortrait ? portraitWidth : normalWidth);
	map<string, string> subs;
	GameData::GetTextReplacements().Substitutions(subs);
	player.AddPlayerSubstitutions(subs);
	newsMessage.Wrap(Format::Replace(news->Message(), subs));
}



void SpaceportPanel::Step()
{
	if(GetUI()->IsTop(this) && port.HasService(Port::ServicesType::OffersMissions))
	{
		Mission *mission = player.MissionToOffer(Mission::SPACEPORT);
		// Special case: if the player somehow got to the spaceport before all
		// landing missions were offered, they can still be offered here:
		if(!mission)
			mission = player.MissionToOffer(Mission::LANDING);
		if(mission)
			mission->Do(Mission::OFFER, player, GetUI());
		else
			player.HandleBlockedMissions(Mission::SPACEPORT, GetUI());
	}
}



void SpaceportPanel::Draw()
{
	if(player.IsDead())
		return;

	// The description text needs to be updated, because player conditions can be changed
	// in the meantime, for example if the player accepts a mission on the Job Board.
	description->SetText(port.Description().ToString());

	if(hasNews)
	{
		const Interface *newsUi = GameData::Interfaces().Get(Screen::Width() < 1280 ?
			"news (small screen)" : "news");
		newsUi->Draw(newsInfo);
		// Depending on if the news has a portrait, the interface box that
		// gets filled in changes.
		newsMessage.Draw(newsUi->GetBox(hasPortrait ? "message portrait" : "message").TopLeft(),
			*GameData::Colors().Get("medium"));
	}
}



void SpaceportPanel::Resize()
{
	const Interface *ui = GameData::Interfaces().Get(Screen::Width() < 1280 ?
		"spaceport (small screen)" : "spaceport");
	description->SetRect(ui->GetBox("content"));
}



// Pick a random news object that applies to the player's planets and conditions.
// If there is no applicable news, this returns null.
const News *SpaceportPanel::PickNews() const
{
	if(!port.HasNews())
		return nullptr;

	vector<const News *> matches;
	const Planet *planet = player.GetPlanet();
	for(const auto &it : GameData::SpaceportNews())
		if(!it.second.IsEmpty() && it.second.Matches(planet))
			matches.push_back(&it.second);

	return matches.empty() ? nullptr : matches[Random::Int(matches.size())];
}
