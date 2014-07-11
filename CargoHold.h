/* CargoHold.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef CARGO_HOLD_H_
#define CARGO_HOLD_H_

#include "DataFile.h"

#include <map>
#include <string>

class GameData;
class Outfit;
class System;



class CargoHold {
public:
	CargoHold();
	
	void Clear();
	
	// Load the cargo manifest from a DataFile. This must be done after the
	// GameData is loaded, so that the sizes of any outfits are known.
	void Load(const DataFile::Node &node, const GameData &data);
	// Save the cargo manifest to a file. Optionally prefix each line with the
	// given number of tabs, if this CargoHold is inside a larger class.
	void Save(std::ostream &out, int depth = 0) const;
	
	// Set the capacity of this cargo hold.
	void SetSize(int tons);
	int Size() const;
	int Free() const;
	int Used() const;
	int CommoditiesSize() const;
	int OutfitsSize() const;
	bool HasOutfits() const;
	
	// Normal cargo:
	int Get(const std::string &commodity) const;
	// Spare outfits:
	int Get(const Outfit *outfit) const;
	// TODO: mission-specific cargo.
	
	const std::map<std::string, int> &Commodities() const;
	const std::map<const Outfit *, int> &Outfits() const;
	
	// For all the transfer functions, the "other" can be null if you simply want
	// the commodity to "disappear" or, if the "amount" is negative, to have an
	// unlimited supply. The return value is the actual number transferred.
	int Transfer(const std::string &commodity, int amount, CargoHold *to = nullptr);
	int Transfer(const Outfit *outfit, int amount, CargoHold *to = nullptr);
	// Transfer as much as the given cargo hold has capacity for. The priority is
	// first mission cargo, then spare outfits, then ordinary commodities.
	void TransferAll(CargoHold *to);
	
	// Get the total value of all this cargo, in the given system.
	int Value(const System *system) const;
	
	
private:
	int size;
	// The amount of cargo used is calculated the first time it is asked for, so
	// that the outfits can be set before those outfit objects are loaded.
	mutable int used;
	std::map<std::string, int> commodities;
	std::map<const Outfit *, int> outfits;
};



#endif
