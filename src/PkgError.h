/* ------------------------------------------------------------------------------
 * Copyright (c) 2007 Novell, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may find
 * current contact information at www.novell.com.
 * ------------------------------------------------------------------------------
 */

/*
   File:	$Id$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Class representing an error message
   Namespace:   Pkg
*/


#ifndef PkgError_h
#define PkgError_h

#include <string>

class PkgError
{
    private:
      std::string _last_error;
      std::string _last_error_details;

    public:
      PkgError() : _last_error(), _last_error_details() {}

      void setLastError(const std::string &error = "", const std::string &details = "");
      const std::string& lastError() {return _last_error;}
      const std::string& lastErrorDetails() {return _last_error_details;}
};

#endif

