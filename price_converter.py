# Python script to convert all the commodity prices in the map file.  



originalPrices = {	"Food":(100,600),
			"Clothing": (140, 440),
                        "Metal": (190, 590),
                        "Plastic": (240, 540),
                        "Equipment": (330, 730),
                        "Medical": (430, 930),
                        "Industrial": (520, 920),
                        "Electronics": (590, 890),
                        "\"Heavy Metals\"": (610, 1310),
                        "\"Luxury Goods\"": (920, 1520)
		}

# Open the map file
fin = open("map_old.txt")
fout = open("map.txt", "w")

for line in fin:
        newLine = line
        if line.startswith("\ttrade"):
                # Get the commodity and the price, convert the price, write the line back with a modified value.
                split = line.split()
                if (split[-2] != "percentile"):
                        price = split[-1]
                        commodity = ""
                        for word in split[1:-1]:
                                commodity += word + " "
                        
                        lowBar, highBar = originalPrices[commodity.strip()]
                        priceRating = ((float(price) - lowBar)*100.0) / (highBar - lowBar)
                        #print ("low=" + str(lowBar) + " high=" + str(highBar) + " price=" + price)
                        newLine = "\ttrade %spercentile %.2f\n" % (commodity, priceRating)
                        #print(newLine)
        fout.write(newLine) 

fin.close()
fout.close()
print("Done!")
