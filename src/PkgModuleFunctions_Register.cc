

#include <ycp/YCPBoolean.h>
#include <ycp/YCPValue.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPVoid.h>

#include "PkgModuleFunctions.h"

void PkgModuleFunctions::registerFunctions()
{
#include "PkgBuiltinTable.h"
}


