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
   Summary:     Functions related to error handling
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "log.h"

#include "Callbacks.h"

#include <ycp/YCPInteger.h>
#include <ycp/YCPString.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPVoid.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPList.h>

#include <zypp/ZYppFactory.h>

// sleep
#include <unistd.h>

// textdomain
#include <libintl.h>                                                                                                                                           
/*
  Textdomain "pkg-bindings"
*/

/////////////////////////////////////////////////////////////////////////////


const zypp::ResStatus::TransactByValue PkgFunctions::whoWantsIt = zypp::ResStatus::APPL_HIGH;

/**
 * Constructor.
 */
PkgFunctions::PkgFunctions () :
      _target_root( "/" )
    , _target_loaded(false)
    , zypp_pointer(NULL)
    , commit_policy(NULL)
    ,_callbackHandler( *new CallbackHandler(*this) )
    , base_product(NULL)
{
    const char *domain = "pkg-bindings";
    bindtextdomain( domain, LOCALEDIR );
    bind_textdomain_codeset( domain, "utf8" );
    textdomain( domain );

    // Make change known.
    {
	extern int _nl_msg_cat_cntr;
	++_nl_msg_cat_cntr;
    }
}

/**
 * Connect to Zypp library if it is not already connected.
 */
zypp::ZYpp::Ptr PkgFunctions::zypp_ptr()
{
    if (zypp_pointer != NULL)
    {
	return zypp_pointer;
    }

    int max_count = 5;
    unsigned int seconds = 3;

    while (zypp_pointer == NULL && max_count > 0)
    {
	try
	{
	    y2milestone("Initializing Zypp library...");
	    zypp_pointer = zypp::getZYpp();

 	    // initialize solver flag, be compatible with zypper
	    zypp_pointer->resolver()->setIgnoreAlreadyRecommended(true);

	    return zypp_pointer;
	}
	catch (const zypp::Exception &excpt)
	{
	    // is it the last attempt?
	    if (max_count == 1)
	    {
		ZYPP_RETHROW(excpt);
	    }
	}

	max_count--;

	if (zypp_pointer == NULL && max_count > 0)
	{
	    sleep(seconds);
	}
    }

    if (zypp_pointer == NULL)
    {
	// still not initialized, throw an exception
	// translators: this is an error message
	ZYPP_THROW (zypp::Exception(std::string(_("Cannot connect to the package manager"))));
    }

    return zypp_pointer;
}


/**
 * Destructor.
 */
PkgFunctions::~PkgFunctions ()
{
    delete &_callbackHandler;

    if (base_product)
    {
	base_product = NULL;
    }

    if (zypp_pointer != NULL)
    {
	y2milestone("Releasing the zypp pointer...");
	zypp_pointer = NULL;
	y2milestone("Zypp pointer released");
    }
}

// ------------------------------------------------------------------
// general

/**
 * @builtin Connect
 * @short Explicitly connect to the package manager, if it is already connected do nothing
 * @description Checks whether the package manager is connected to Yast,
 * if not try connecting to it.
 * @return boolean true if the package manager is connected
 */
YCPValue
PkgFunctions::Connect()
{
    try
    {
	return YCPBoolean(zypp_ptr() != NULL);
    }
    catch(const zypp::ZYppFactoryException &excpt)
    {
	y2error("Error in Connect: FactoryException: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asString());
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Error in Connect: Exception: %s", excpt.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
    }

    return YCPBoolean(false);
}

/**
 * @builtin LastError
 *
 * @short get current error as string
 * @return string
 */
YCPValue
PkgFunctions::LastError ()
{
    return YCPString(_last_error.lastError());
}

/**
 * @builtin LastErrorDetails
 *
 * @short get current error details as string
 * @return string Error Details
 */
YCPValue
PkgFunctions::LastErrorDetails ()
{
    return YCPString (_last_error.lastErrorDetails());
}

/**
 * @builtin LastErrorId
 * @short Obsoleted function, do not use
 * @return string
 */
YCPValue
PkgFunctions::LastErrorId ()
{

    /* TODO FIXME
    int errorId = _last_error;
    switch ( errorId ) {
        case PMError::E_ok:
            return YCPString( "ok" );
        case InstSrcError::E_isrc_cache_duplicate:
            return YCPString( "instsrc_duplicate" );
        default:
            return YCPString( "error" );
    }
    */

    return YCPString( "ok" );
}

/**
 * @builtin Init
 * @short completely initialize package management (currently it is empty)
 * @return boolean true on success
 */
YCPValue
PkgFunctions::Init ()
{
#warning  FIXME can be Init() empty??
    return YCPBoolean(true);
}

zypp::RepoManager PkgFunctions::CreateRepoManager()
{
    // set path option, use root dir as a prefix for the default directory
    zypp::RepoManagerOptions repo_options(_target_root);

    y2milestone("Path to repository files: %s", repo_options.knownReposPath.asString().c_str());

    return zypp::RepoManager(repo_options);
}

// convert Exception object to string represenatation
std::string PkgFunctions::ExceptionAsString(const zypp::Exception &e)
{
    std::string ret = e.asUserString();

    if (e.historySize() > 0)
    {
	ret += "\n" + e.historyAsString();
    }

    y2debug("Error message: %s", ret.c_str());

    return ret;
}

PkgFunctions::RepoId PkgFunctions::LastReportedRepo() const
{
    return last_reported_repo;
}

int PkgFunctions::LastReportedMedium() const
{
    return last_reported_mediumnr;
}

void PkgFunctions::SetReportedSource(RepoId repo, int medium)
{
    last_reported_repo = repo;
    last_reported_mediumnr = medium;
}

/**
 * @builtin ZConfig
 * @short get the current libzypp configuration
 * @return map<string,any> libzypp configuration map
 */
YCPValue PkgFunctions::ZConfig()
{
    zypp::ZConfig &zconfig = zypp::ZConfig::instance();

    YCPMap ret;

    ret->add(YCPString("repo_cache_path"), YCPString(zconfig.repoCachePath().asString()));
    ret->add(YCPString("repo_metadata_path"), YCPString(zconfig.repoMetadataPath().asString()));
    ret->add(YCPString("repo_solv_files_path"), YCPString(zconfig.repoSolvfilesPath().asString()));
    ret->add(YCPString("repo_packages_path"), YCPString(zconfig.repoPackagesPath().asString()));

    ret->add(YCPString("config_path"), YCPString(zconfig.configPath().asString()));

    ret->add(YCPString("known_repos_path"), YCPString(zconfig.knownReposPath().asString()));
    ret->add(YCPString("known_services_path"), YCPString(zconfig.knownServicesPath().asString()));

    ret->add(YCPString("repo_add_probe"), YCPBoolean(zconfig.repo_add_probe()));
    ret->add(YCPString("repo_refresh_delay"), YCPInteger(zconfig.repo_refresh_delay()));
    ret->add(YCPString("repo_label_is_alias"), YCPBoolean(zconfig.repoLabelIsAlias()));

    ret->add(YCPString("download_max_concurrent_connections"), YCPInteger(zconfig.download_max_concurrent_connections()));
    ret->add(YCPString("download_min_download_speed"), YCPInteger(zconfig.download_min_download_speed()));
    ret->add(YCPString("download_max_download_speed"), YCPInteger(zconfig.download_max_download_speed()));
    ret->add(YCPString("download_max_silent_tries"), YCPInteger(zconfig.download_max_silent_tries()));
    ret->add(YCPString("download_use_deltarpm"), YCPBoolean(zconfig.download_use_deltarpm()));
    ret->add(YCPString("download_use_deltarpm_always"), YCPBoolean(zconfig.download_use_deltarpm_always()));
    ret->add(YCPString("download_media_prefer_download"), YCPBoolean(zconfig.download_media_prefer_download()));
    ret->add(YCPString("download_media_prefer_volatile"), YCPBoolean(zconfig.download_media_prefer_volatile()));

    const char* dmode;
    zypp::DownloadMode dm = zconfig.commit_downloadMode();
    
    if (dm == zypp::DownloadDefault) dmode = "default";
    else if (dm == zypp::DownloadInAdvance) dmode = "download_in_advance";
    else if (dm == zypp::DownloadAsNeeded) dmode = "download_as_needed";
    else if (dm == zypp::DownloadInHeaps) dmode = "download_in_heaps";
    else if (dm == zypp::DownloadOnly) dmode = "download_only";
    else dmode = "unknown";

    ret->add(YCPString("download_mode"), YCPSymbol(dmode));

    ret->add(YCPString("system_root"), YCPString(zconfig.systemRoot().asString()));
    ret->add(YCPString("system_architecture"), YCPString(zconfig.systemArchitecture().asString()));
    ret->add(YCPString("system_default_architecture"), YCPString(zconfig.defaultSystemArchitecture().asString()));

    ret->add(YCPString("text_locale_default"), YCPString(zconfig.defaultTextLocale().code()));
    ret->add(YCPString("text_locale"), YCPString(zconfig.textLocale().code()));

    ret->add(YCPString("vendor_path"), YCPString(zconfig.vendorPath().asString()));

    ret->add(YCPString("solver_only_requires"), YCPBoolean(zconfig.solver_onlyRequires()));
    ret->add(YCPString("solver_allow_vendor_change"), YCPBoolean(zconfig.solver_allowVendorChange()));
    ret->add(YCPString("solver_cleandeps_on_remove"), YCPBoolean(zconfig.solver_cleandepsOnRemove()));
    ret->add(YCPString("solver_upgrade_testcases_to_keep"), YCPBoolean(zconfig.solver_upgradeTestcasesToKeep()));
    ret->add(YCPString("solver_upgrade_remove_dropped_packages"), YCPBoolean(zconfig.solverUpgradeRemoveDroppedPackages()));
    ret->add(YCPString("solver_check_system_file"), YCPString(zconfig.solver_checkSystemFile().asString()));

    ret->add(YCPString("locks_file"), YCPString(zconfig.locksFile().asString()));
    ret->add(YCPString("locks_file_apply"), YCPBoolean(zconfig.apply_locks_file()));

    ret->add(YCPString("update_data_path"), YCPString(zconfig.update_dataPath().asString()));
    ret->add(YCPString("update_messages_path"), YCPString(zconfig.update_messagesPath().asString()));
    ret->add(YCPString("update_scripts_path"), YCPString(zconfig.update_scriptsPath().asString()));
    ret->add(YCPString("update_messages_notify"), YCPString(zconfig.updateMessagesNotify()));

    YCPList rpm_flags;
    zypp::target::rpm::RpmInstFlags flags = zconfig.rpmInstallFlags();

    if (flags.testFlag(zypp::target::rpm::RPMINST_EXCLUDEDOCS)) rpm_flags->add(YCPString("--excludedocs"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_NOSCRIPTS)) rpm_flags->add(YCPString("--noscripts"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_FORCE)) rpm_flags->add(YCPString("--force"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_IGNORESIZE)) rpm_flags->add(YCPString("--ignoresize"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_JUSTDB)) rpm_flags->add(YCPString("--justdb"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_NODEPS)) rpm_flags->add(YCPString("--nodeps"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_NODIGEST)) rpm_flags->add(YCPString("--nodigest"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_NOSIGNATURE)) rpm_flags->add(YCPString("--nosignature"));
    if (flags.testFlag(zypp::target::rpm::RPMINST_TEST)) rpm_flags->add(YCPString("--test"));

    if (flags.testFlag(zypp::target::rpm::RPMINST_NOUPGRADE))
        rpm_flags->add(YCPString("-i"));
    else
        rpm_flags->add(YCPString("-U"));

    ret->add(YCPString("rpm_install_flags"), rpm_flags);
    
    ret->add(YCPString("history_log_file"), YCPString(zconfig.historyLogFile().asString()));

    ret->add(YCPString("credentials_global_dir"), YCPString(zconfig.credentialsGlobalDir().asString()));
    ret->add(YCPString("credentials_global_file"), YCPString(zconfig.credentialsGlobalFile().asString()));

    ret->add(YCPString("plugins_path"), YCPString(zconfig.pluginsPath().asString()));

    ret->add(YCPString("distro_version_pkg"), YCPString(zconfig.distroverpkg()));

    return ret;
}
