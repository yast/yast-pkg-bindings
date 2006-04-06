
#include "PkgError.h"

// helper function
void PkgError::setLastError(const std::string &error, const std::string &details)
{
    _last_error = error;
    _last_error_details = details;
}

