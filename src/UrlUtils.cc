/* ------------------------------------------------------------------------------
 * Copyright (c) 2016 SUSE LLC
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * ------------------------------------------------------------------------------
 */

#include "PkgFunctions.h"
#include "log.h"

#include <zypp/Url.h>

#include <ycp/YCPString.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPList.h>

/**
 * @builtin UrlKnownSchemes
 * @description Returns all URL schemes known to libzypp.
 * @note The list contains *all* schemes which are known to libzypp.
 *   The list contains also unsupported schemes (e.g. "sftp" which can be used
 *   in repository URL, but the respective downloader is not implemented) or
 *   schemes which not make sense for repositories (e.g. "mailto" or "ldap").
 * @short URL schemes known by libzypp
 * @return list of URL schemes
 */
YCPValue PkgFunctions::UrlKnownSchemes()
{
    YCPList list;
    zypp::url::UrlSchemes schemes = zypp::Url::getRegisteredSchemes();

    for(auto&& str: schemes) list->add(YCPString(str));

    return list;
}

/**
 * @builtin UrlSchemeIsRemote
 * @short Is the URL scheme remote?
 * @return true if the scheme is remote, false otherwise
 */
YCPValue PkgFunctions::UrlSchemeIsRemote(const YCPString &url_scheme)
{
    return YCPBoolean(zypp::Url::schemeIsRemote(url_scheme->value()));
}

/**
 * @builtin UrlSchemeIsLocal
 * @short Is the URL scheme local?
 * @return true if the scheme is local, false otherwise
 */
YCPValue PkgFunctions::UrlSchemeIsLocal(const YCPString &url_scheme)
{
    return YCPBoolean(zypp::Url::schemeIsLocal(url_scheme->value()));
}

/**
 * @builtin UrlSchemeIsVolatile
 * @short Is the URL scheme volatile? I.e. the content can be changed by changing
 * the medium in the drive even if the URL itself is unchanged.
 * @return true if the scheme is volatile, false otherwise
 */
YCPValue PkgFunctions::UrlSchemeIsVolatile(const YCPString &url_scheme)
{
    return YCPBoolean(zypp::Url::schemeIsVolatile(url_scheme->value()));
}

/**
 * @builtin UrlSchemeIsDownloading
 * @short Do the files need to be downloaded locally before use or can be accessed directly?
 * @return true if the scheme needs download, false otherwise
 */
YCPValue PkgFunctions::UrlSchemeIsDownloading(const YCPString &url_scheme)
{
    return YCPBoolean(zypp::Url::schemeIsDownloading(url_scheme->value()));
}

