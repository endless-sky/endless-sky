/* ShipModel.h
Copyright (c) 2022 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef SHIP_TEMPLATE_H_
#define SHIP_TEMPLATE_H_

#include <string>

class DataWriter;



// Class representing a model or a template of a ship that describes all
// "static" properties and "default" properties of a ship or set of ships,
// like the base-attributes that a ship has and the default Outfits with
// which the ship would be installed.
// Instances of this class don't contain runtime information like ship-
// positions or actual ship-stat values.
class ShipModel {
public:
	// Get the name of this ship template / model / variant.
	const std::string &Name() const;

	// Check that this template and all the outfits and other data it depends upon
	// are correctly loaded.
	bool IsValid() const;

	// Save the full model, as currently configured.
	void Save(DataWriter &out) const;
};



#endif
