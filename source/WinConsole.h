/* WinConsole.h
Copyright (c) 2017 by Frederick Goy IV

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef WIN_CONSOLE_H_
#define WIN_CONSOLE_H_

#include <windows.h>

#include <string>



class WinConsole {
public:
	static void Init();
	
	
private:
	static void WriteConsoleLog(const std::string &message);
};
	


#endif
