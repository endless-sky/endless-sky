/* OptionalInputDialogPanel.h
Copyright (c) 2026 by Amazinite

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

#include "DialogPanel.h"

#include "text/Format.h"
#include "text/Truncate.h"

#include <functional>
#include <optional>
#include <string>



// A special version of DialogPanel for requesting optional values.
// Has a third button named "Unset" that provides an empty optional to the given callback.
class OptionalInputDialogPanel : public DialogPanel {
public:
	template<class T>
	static OptionalInputDialogPanel *RequestInteger(T *t, void (T::*fun)(std::optional<int>),
		std::string message,
		std::optional<int> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);
	template<class T>
	static OptionalInputDialogPanel *RequestDouble(T *t, void (T::*fun)(std::optional<double>),
		std::string message,
		std::optional<double> initialValue = std::nullopt,
		Truncate truncate = Truncate::NONE,
		bool allowsFastForward = false);


private:
	explicit OptionalInputDialogPanel(DialogInit &init, std::function<void(std::optional<int>)> intFun,
		std::function<void(std::optional<double>)> doubleFun);
	bool Unset(const std::string &);

	std::function<void(std::optional<int>)> optionalIntFun;
	std::function<void(std::optional<double>)> optionalDoubleFun;
};



template<class T>
OptionalInputDialogPanel *OptionalInputDialogPanel::RequestInteger(T *t, void (T::*fun)(std::optional<int>),
	std::string message, std::optional<int> initialValue, Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	if(initialValue.has_value())
		init.initialValue = std::to_string(initialValue.value());
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new OptionalInputDialogPanel(init, std::bind(fun, t, std::placeholders::_1), nullptr);
}



template<class T>
OptionalInputDialogPanel *OptionalInputDialogPanel::RequestDouble(T *t, void (T::*fun)(std::optional<double>),
	std::string message, std::optional<double> initialValue, Truncate truncate, bool allowsFastForward)
{
	DialogInit init;
	init.message = std::move(message);
	if(initialValue.has_value())
		init.initialValue = Format::StripCommas(Format::Number(initialValue.value(), 5));
	init.truncate = truncate;
	init.allowsFastForward = allowsFastForward;
	return new OptionalInputDialogPanel(init, nullptr, std::bind(fun, t, std::placeholders::_1));
}
