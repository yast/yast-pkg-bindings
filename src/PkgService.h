/* ------------------------------------------------------------------------------
 * Copyright (c) 2008 Novell, Inc. All Rights Reserved.
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

#include <string>
#include <zypp/ServiceInfo.h>

// Extend zypp::ServiceInfo class to contain "deleted" flag and original service alias
class PkgService : public zypp::ServiceInfo
{
    public:

	PkgService();

	PkgService(const zypp::ServiceInfo &s, const std::string &old_alias = "");

	PkgService(const PkgService &s);

	~PkgService();

	bool isDeleted() const;

	void setDeleted();

	std::string origAlias() const;


    private:

	bool _deleted;
	std::string _old_alias;
};


