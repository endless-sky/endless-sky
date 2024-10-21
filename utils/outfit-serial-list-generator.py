#!/usr/bin/python
# outfit-serial-list-generator.py
# Copyright (c) 2024 by Daeridanii
#
# Endless Sky is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later version.
#
# Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

categories = [
	"Guns",
	"Turrets",
	"Secondary Weapons",
	"Ammunition",
	"Systems",
	"Power",
	"Engines",
	"Hand to Hand",
	"Unique",
	"Licenses"
]

turrets_series = [
	"Turrets",
	"Anti-Missile",
]

systems_series = [
	"$Drives", # The $ creates a single category shared by all species.
	"Shields",
	"Repair",
	"Cooling",
	"Scanners",
	"Jammers",
	"Ramscoops",
	"Special Systems",
	"$Fuel",
	"$Passenger",
	"%Utility", # The % lists this with index 999, so it comes after all other subseries.
]

power_series = [
	"Generators",
	"Batteries",
	"Solar",
]

engines_series = [
	"Engines",
	"Afterburners",
]

h2h_series = [
	"H2H",
	"Fortifications",
]

unique_series = [
	"$Functional Unique",
	"$Non-Functional Unique",
]

species = [
	"Human",
	"Hai",
	"Remnant",
	"Korath",
	"Ka'het",
	"Gegno",
	"Coalition",
	"Wanderers",
	"Bunrodea",
	"Successors",
	"Incipias",
	"Sheragi",
	"Pug",
	"Quarg",
	"%Other",
	]

def iterate(category_series, s):
	s = ""
	label = 1

	for series in category_series:
		if series[0] == "$":
			index = label * 100
			s += '\t"' + (series.replace("$","") + '" '+ str(index) + "\n")
		elif series[0] == "%":
			index = 9999
			s += '\t"' + (series.replace("%","") + '" '+ str(index) + "\n")
		else:
			for spec in species:
				if spec[0] == "%":
					index = (100 * label) + 99
				else:
					index = (100 * label) + species.index(spec)

				s += '\t"' + (series + ": " + str(spec.replace("%","")) + '" ' + str(index) + "\n")
		label += 1
	return s

def iterate_singular(name, s):
	s = ""

	ser = 0
	for spec in species:
		if spec[0] == "%":
			index = 99
		else:
			index = species.index(spec)

		s += '\t"' + (name + ": " + str(spec.replace("%","")) + '" ' + str(index) + "\n")
	return s

f = open("series.txt", "w")
data = 'category "series"\n'
for category in categories:
	data += "\t"
	data += ("# " + category + "\n") # Add comment tags

	match category:
		case "Guns":                data += iterate_singular("Guns", data)
		case "Turrets":             data += iterate(turrets_series, data)
		case "Secondary Weapons":   data += iterate_singular("Secondary Weapons", data)
		case "Ammunition":          data += iterate_singular("Ammunition", data)
		case "Systems":             data += iterate(systems_series, data)
		case "Power":               data += iterate(power_series, data)
		case "Engines":             data += iterate(engines_series, data)
		case "Hand to Hand":        data += iterate(h2h_series, data)
		case "Unique":              data += iterate(unique_series, data)
		case "Licenses":            data += iterate_singular("Licenses", data)

f.write(data)
f.close()
