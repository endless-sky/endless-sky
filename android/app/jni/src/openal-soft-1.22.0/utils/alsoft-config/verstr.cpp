
#include "verstr.h"

#include "version.h"


QString GetVersionString()
{
    return QStringLiteral(ALSOFT_VERSION "-" ALSOFT_GIT_COMMIT_HASH " (" ALSOFT_GIT_BRANCH " branch).");
}
