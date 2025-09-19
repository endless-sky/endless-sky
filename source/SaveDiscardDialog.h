// /* SaveDiscardDialog.h
// Copyright (c) 2025 by xobes
//
// Endless Sky is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later version.
//
// Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.
// */
//
// #pragma once
//
// #include "Dialog.h"
//
// #include <string>
//
//
//
// // A special version of Dialog with three buttons: [Discard] [Cancel] [Save]
// class SaveDiscardDialog : public Dialog {
// public:
// 	template<class T>
// 	SaveDiscardDialog(T *panel,
// 		void (T::*saveFun)(const std::string &),
// 		std::function<bool(const std::string &)> validate,
// 		void (T::*discardFun)(),
// 		const std::string &message,
// 		std::string initialValue = "");
//
// private:
// 	virtual bool ThirdButtonFun(std::string &input) override;
// 	std::function<void()> discardAction;
// 	// TODO: If the player enters a filename that exists, prompt before overwriting it.
// 	std::string nameToConfirm;
// };
//
//
//
// template<class T>
// SaveDiscardDialog::SaveDiscardDialog(T *panel,
// 		void (T::*saveAction)(const std::string &),
// 		std::function<bool(const std::string &)> validate,
// 		void (T::*discardFun)(),
// 		const std::string &message,
// 		std::string initialValue)
// 	: Dialog(panel, saveAction, message, validate, initialValue)
// {
// 	discardAction = std::bind(discardFun, panel);
// 	okText = "Save";
// 	thirdButtonLabel = "Discard";
// 	numButtons = 3;
// }
//
//
// Dialog::FunctionButton(this, "_Save", 's', &PreferencesPanel::SaveFunction);
// Dialog::FunctionButton(this, "_Cancel", 'c', &PreferencesPanel::CancelFunction)
// Dialog::FunctionButton(this, "_Discard", 'd', &PreferencesPanel::DiscardFunction)
