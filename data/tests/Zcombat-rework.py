import re, os, sys

nargs = len(sys.argv)

file_in = sys.argv[1]
file_read = open(file_in, 'r')
output = open(file_in + ".out", 'w')
full = file_read.readlines()
for line in full:
    if(line.startswith('\t\t"lifetime"')):
        output.write('\t\t"lifetime" ' + str((float(line[12:]) * 2)) + "\n")
    elif(line.startswith('\t\t"velocity"')):
        output.write('\t\t"velocity" ' + str((float(line[12:]) / 2)) + "\n")
    elif(line.startswith('\t\t"inaccuracy"')):
        output.write('\t\t"inaccuracy" ' + str((float(line[14:]) * 2)) + "\n")
    elif(line.startswith('\t\t"reload"')):
        output.write('\t\t"reload" ' + str((float(line[10:]) * 2)) + "\n")
    elif(line.startswith('\t\t"firing energy"')):
        output.write('\t\t"firing energy" ' + str((float(line[18:]) * 2)) + "\n")
    elif(line.startswith('\t\t"firing heat"')):
        output.write('\t\t"firing heat" ' + str((float(line[16:]) * 2)) + "\n")
    elif(line.startswith('\t\t"shield damage"')):
        output.write('\t\t"shield damage" ' + str((float(line[18:]) * 2)) + "\n")
    elif(line.startswith('\t\t"hull damage"')):
        output.write('\t\t"hull damage" ' + str((float(line[16:]) * 2)) + "\n")
    elif(line.startswith('\t\t"turret turn"')):
        output.write('\t\t"turret turn" ' + str((float(line[16:]) / 2)) + "\n")
    elif(line.startswith('\t\t"hit force"')):
        output.write('\t\t"hit force" ' + str((float(line[14:]) * 2)) + "\n")
    elif(line.startswith('\t\t"firing force"')):
        output.write('\t\t"firing force" ' + str((float(line[17:]) * 2)) + "\n")
    elif(line.startswith('\t\t"ion damage"')):
        output.write('\t\t"ion damage" ' + str((float(line[15:]) * 2)) + "\n")
    elif(line.startswith('\t\t"heat damage"')):
        output.write('\t\t"heat damage" ' + str((float(line[16:]) * 2)) + "\n")
    elif(line.startswith('\t\t"slowing damage"')):
        output.write('\t\t"slowing damage" ' + str((float(line[19:]) * 2)) + "\n")
    else:
        output.write(line)
file_read.close