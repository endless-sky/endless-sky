/* Font.h
Copyright (c) 2014-2020 by Michael Zahniser

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

#include "../shader/Shader.h"

#include "../opengl.h"

#include <filesystem>
#include <functional>
#include <string>

class Color;
class DisplayText;
class ImageBuffer;
class Point;



// Class for drawing text in OpenGL. Each font is based on a single image with
// glyphs for each character in ASCII order (not counting control characters).
// The kerning between characters is automatically adjusted to look good. At the
// moment only plain ASCII characters are supported, not Unicode.
class Font {
public:
	Font() noexcept = default;
	explicit Font(const std::filesystem::path &imagePath);

	void Load(const std::filesystem::path &imagePath);

	// Draw a text string, subject to the given layout and truncation strategy.
	void Draw(const DisplayText &text, const Point &point, const Color &color) const;
	void DrawAliased(const DisplayText &text, double x, double y, const Color &color) const;
	// Draw the given text string, e.g. post-formatting (or without regard to formatting).
	void Draw(const std::string &str, const Point &point, const Color &color) const;
	void DrawAliased(const std::string &str, double x, double y, const Color &color) const;

	// Determine the string's width, without considering formatting.
	int Width(const std::string &str, char after = ' ') const;
	// Get the width of the text while accounting for the desired layout and truncation strategy.
	int FormattedWidth(const DisplayText &text, char after = ' ') const;

	int Height() const noexcept;

	int Space() const noexcept;

	static void ShowUnderlines(bool show) noexcept;


private:
	static int Glyph(char c, bool isAfterSpace) noexcept;
	void LoadTexture(ImageBuffer &image);
	void CalculateAdvances(ImageBuffer &image);
	void SetUpShader(float glyphW, float glyphH);

	int WidthRawString(const char *str, char after = ' ') const noexcept;

	std::string TruncateText(const DisplayText &text, int &width) const;
	std::string TruncateBack(const std::string &str, int &width) const;
	std::string TruncateFront(const std::string &str, int &width) const;
	std::string TruncateMiddle(const std::string &str, int &width) const;

	std::string TruncateEndsOrMiddle(const std::string &str, int &width,
		std::function<std::string(const std::string &, int)> getResultString) const;

private:
	const Shader *shader;
	GLuint texture = 0;

	int height = 0;
	int space = 0;
	mutable int screenWidth = 0;
	mutable int screenHeight = 0;
	mutable GLfloat scale[2]{0.f, 0.f};
	GLfloat glyphWidth = 0.f;
	GLfloat glyphHeight = 0.f;

	static const int GLYPHS = 98;
	int advance[GLYPHS * GLYPHS] = {};
	int widthEllipses = 0;
};
