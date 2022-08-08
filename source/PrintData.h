/* PrintData.h
Copyright (c) 2022 by warp-core

Main function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PRINT_DATA_H_
#define PRINT_DATA_H_

class PrintData {
public:
	static void Print(char *argv[]);


private:
	static void Help();

	static void Ships(char *argv[]);
	static void PrintShipBaseStats();
	static void PrintShipDeterrences(bool variants = false);
	static void PrintShipShipyards();
	static void PrintShipOldTable();

	static void Weapons(char *argv[]);
	static void PrintWeaponStats();
	static void PrintWeaponDeterrences();

	static void PrintEngineStats();

	static void PrintPowerStats();

	static void Outfits(char *argv[]);
	static void PrintOutfitsList();
	static void PrintOutfitOutfitters();
	static void PrintOutfitsAllStats();
};

#endif
