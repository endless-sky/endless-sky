/* Permissions.h
Copyright (c) 2022 by Lemuria

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ESKY_PERMISSIONS_H_
#define ESKY_PERMISSIONS_H_

#include "DataNode.h"

class Permissions {
public:
	bool CanSell();
	bool CanUninstall();

public:
	void Load (const DataNode &node);

private:
	bool canSell;
	bool canUninstall;
};

#endif
