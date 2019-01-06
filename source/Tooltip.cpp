#include "Tooltip.h"

#include "Outfit.h"

#include <string>
#include <vector>
#include <math.h>

double getOutfitValue(const Outfit & outfit, const std::string & name) {
	double gunValue = outfit.IsWeapon() ? outfit.WeaponValue(name) : 0.0;
	double attributeValue = outfit.Attributes().Get(name);

	
	auto sit = Outfit::SCALE.find(name);
	double scale = (sit == Outfit::SCALE.end() ? 1. : sit->second);

	return ( gunValue != 0.0 ? gunValue : attributeValue ) * scale;
}

Tooltip::Tooltip() : text(""), compareTo(0), addSummary(false), label("")
{}

Tooltip::Tooltip(const std::string label) : text(""), compareTo(0), addSummary(false), label(label)
{}

const std::string& Tooltip::Text() const
{
	return text;
}

const bool Tooltip::HasExtras() const
{
	return compareTo.size() > 0;
}

const std::string Tooltip::ExtrasForOutfit(const Outfit & outfit) const
{	
	const std::string baseAttributeName = label.substr(0, label.size() - 1);	
	const double baseValue = getOutfitValue(outfit, baseAttributeName);
	
	if (baseValue != 0.0) {
		std::string extraText = "\n" + baseAttributeName + " efficiency per:";
		
		double attributeProduct = 1.0;
		int presentAttributes = 0;
		
		for (const std::string& attribute : compareTo) {
			double attributeValue = abs(getOutfitValue(outfit, attribute));
			if (attributeValue != 0.0) {
				presentAttributes++;
				extraText += "\n\t" + attribute + ": " + std::to_string(baseValue/attributeValue);
				attributeProduct *= attributeValue;
			}
		}
		
		if (addSummary && presentAttributes) {
			extraText += "\n\nOverall efficiency: " + std::to_string( pow(baseValue, presentAttributes) / attributeProduct );
		}
		
		return extraText;
	}
	
	return "[NO EXTRA INFO WARNING]";
}

void Tooltip::AddText(std::string newText)
{
	if (!text.empty()) {
		text.append("\n");
		if(newText[0] != '\t')
			text.append("\t");
	}

	text.append(newText);
}

void Tooltip::AddComparison(std::string attributeName)
{
	compareTo.push_back(attributeName);
}

void Tooltip::SetSummary(bool shouldAddSummary)
{;
	addSummary = shouldAddSummary;
}
