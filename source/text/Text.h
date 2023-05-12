/* Text.h
Copyright (c) 2023 by Rian Shelley

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TEXT_H_INCLUDED
#define TEXT_H_INCLUDED

#include <cstdint>
#include <map>
#include <memory>
#include <string>

/// Class used for text meant for display.
/// TODO: Translate the text if a supported PO file is loaded
/// TODO: locale for times/dates/lists
/// TODO: language specific object properties
/// TODO: bidi/rtl text
class Text
{
private:
	// Interface to support converting from any type to a formatted string
	class FormatArg
	{
	public:
		virtual std::string ToString() const = 0;
		virtual int64_t N() const { return -1; }
	};
	// Handle string formatting
	class FormatArgString : public FormatArg
	{
	public:
		FormatArgString(const std::string& s) : m_s(s) {}
		virtual std::string ToString() const override { return m_s; }
	private:
		std::string m_s;
	};
	// Handle number formatting
	class FormatArgInt : public FormatArg
	{
	public:
		FormatArgInt(int64_t i) : m_i(i) {}
		virtual std::string ToString() const override { return std::to_string(m_i); }
		virtual int64_t N() const override { return m_i; }

	private:
		int64_t m_i;
	};
	// TODO dates, lists, anything with a StringTable (which hasn't been written yet)

public:
	class Arg
	{
	public:
		Arg(const std::string& s) { m_a.reset(new FormatArgString(s)); }
		Arg(int64_t i) { m_a.reset(new FormatArgInt(i)); }

	protected:
		std::string ToString() const { return m_a->ToString(); }
		int64_t N() const { return m_a->N(); }

	private:
		std::shared_ptr<FormatArg> m_a;

		friend class Text;
	};

	typedef std::map<std::string, Arg> Args;

	static Text Format(const std::string& format);
	static Text Format(const std::string& format, const Args& args);
	static Text FormatN(const std::string& format_singular, const std::string& format_plural, const Args& args);

	// TODO: return a glyph sequence, not a string.
	const std::string& ToString() const { return m_s; }

	// TODO: additional text properties for formatting (text directionality,
	//	      formatting rules for line breaks, etc)

private:
	Text(const std::string& s): m_s(s) {}

	const std::string m_s;
};

#endif // TEXT_H_INCLUDED
