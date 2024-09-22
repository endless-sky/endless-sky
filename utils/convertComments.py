import os


paths = ["source/", "source/comparators/", "source/ship/", "source/text/"]

for path in paths:
    directory = os.fsencode(path)
    for file in os.listdir(directory):
        filename = os.fsdecode(file)
        if filename.endswith(".h") or filename.endswith(".hpp"):
            lines = None
            with open(path + filename, 'r') as file:
                lines = file.readlines()

            newLines = []
            commentLines = []

            counter = 0

            inCommentBlock = False
            identationLevel = 0
            for line in lines:
                prefix = ""
                identationLevel = len(line) - len(line.lstrip('\t'))
                prefix += "\t" * identationLevel

                if line.startswith(prefix + "//") and not inCommentBlock:
                    counter += 1
                    inCommentBlock = True
                    commentLines.append(line)
                elif not line.startswith(prefix + "//") and inCommentBlock:
                    inCommentBlock = False
                    line_prefix = "\t" * (len(commentLines[0]) - len(commentLines[0].lstrip('\t')))
                    if len(commentLines) == 1:
                        newLines.append(line_prefix + "///\n")
                    for cLine in commentLines:
                        converted = cLine.replace("//", "///")
                        newLines.append(converted)
                    newLines.append(line)
                    commentLines.clear()
                elif inCommentBlock:
                    commentLines.append(line)
                else:
                    newLines.append(line)

            with open(path + filename, 'w') as file:
                for line in newLines:
                    file.write(line)

            print("Converted comments in: " + path + filename + " , " + str(counter) + " comments got converted.")

