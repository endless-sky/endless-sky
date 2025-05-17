/* GameLoadingPanel.cpp
Copyright (c) 2021 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "GameLoadingPanel.h"

#include "Angle.h"
#include "audio/Audio.h"
#include "Color.h"
#include "Conversation.h"
#include "ConversationPanel.h"
#include "GameData.h"
#include "image/MaskManager.h"
#include "MenuAnimationPanel.h"
#include "MenuPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "shader/PointerShader.h"
#include "Ship.h"
#include "image/SpriteSet.h"
#include "shader/StarField.h"
#include "System.h"
#include "TaskQueue.h"
#include "UI.h"

#include "opengl.h"



GameLoadingPanel::GameLoadingPanel(PlayerInfo &player, TaskQueue &queue, const Conversation &conversation,
	UI &gamePanels, bool &finishedLoading)
	: player(player), queue(queue), conversation(conversation), gamePanels(gamePanels),
		finishedLoading(finishedLoading), ANGLE_OFFSET(360. / MAX_TICKS)
{
	SetIsFullScreen(true);
}



void GameLoadingPanel::Step()
{
	progress = static_cast<int>(GameData::GetProgress() * MAX_TICKS);

	queue.ProcessSyncTasks();
	if(GameData::IsLoaded())
	{
		// Now that we have finished loading all the basic sprites and sounds, we can look for invalid file paths,
		// e.g. due to capitalization errors or other typos.
		SpriteSet::CheckReferences();
		Audio::CheckReferences();
		// Set the game's initial internal state.
		GameData::FinishLoading();

		player.LoadRecent();

		// All sprites with collision masks should also have their 1x scaled versions, so create
		// any additional scaled masks from the default one.
		GameData::GetMaskManager().ScaleMasks();

		GetUI()->Pop(this);
		if(conversation.IsEmpty())
		{
			GetUI()->Push(new MenuPanel(player, gamePanels));
			GetUI()->Push(new MenuAnimationPanel());
		}
		else
		{
			GetUI()->Push(new MenuAnimationPanel());

			auto *talk = new ConversationPanel(player, conversation);

			UI *ui = GetUI();
			talk->SetCallback([ui](int response) { ui->Quit(); });
			GetUI()->Push(talk);
		}

		finishedLoading = true;
	}
}



void GameLoadingPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point());

	GameData::DrawMenuBackground(this);

	// Draw the loading circle.
	Angle da(ANGLE_OFFSET);
	Angle a(0.);
	PointerShader::Bind();
	for(int i = 0; i < progress; ++i)
	{
		PointerShader::Add(Point(), a.Unit(), 8.f, 20.f, 140.f, Color(.5f, 0.f));
		a += da;
	}
	PointerShader::Unbind();
}
