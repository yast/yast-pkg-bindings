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
#include <zypp/Edition.h>

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
    , _source_loaded(false)
    , zypp_pointer(NULL)
    , repo_manager(NULL)
    , autorefresh_skipped(false)
    , current_repo(-1LL)
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
        delete base_product;
	base_product = NULL;
    }

    if (repo_manager)
    {
      y2milestone("Releasing the repo manager...");
      delete repo_manager;
      repo_manager = NULL;
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

zypp::RepoManager* PkgFunctions::CreateRepoManager()
{
    if (repo_manager) return repo_manager;

    // set path option, use root dir as a prefix for the default directory
    zypp::RepoManagerOptions repo_manager_options(_target_root);
    y2milestone("Path to repository files: %s", repo_manager_options.knownReposPath.asString().c_str());

    // set the previously configured "target_distro" option
    if(!repo_options->value(YCPString("target_distro")).isNull() && repo_options->value(YCPString("target_distro"))->isString())
    {
        std::string target_distro = repo_options->value(YCPString("target_distro"))->asString()->value();
        y2milestone("Using target_distro: %s", target_distro.c_str());
        repo_manager_options.servicesTargetDistro = target_distro;
    }

    repo_manager = new zypp::RepoManager(repo_manager_options);
    return repo_manager;
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
 * Return current libzypp configuration. See /etc/zypp/zypp.conf for more details
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

/**
 * @builtin SetZConfig
 * @short set the current libzypp configuration
 * This is a set counterpart to Pkg::ZConfig(). Note that the set of options which can be changed is very limited.
 * Currently supported values: $[ "download_media_prefer_download" : boolean,
 * "update_messages_notify" : string,
 * "solver_upgrade_remove_dropped_packages" : boolean ]
 * @return boolean true on success
 */
YCPValue PkgFunctions::SetZConfig(const YCPMap &cfg)
{
    zypp::ZConfig &zconfig = zypp::ZConfig::instance();

    const char *key = "download_media_prefer_download";
    if(!cfg->value(YCPString(key)).isNull())
    {
	const YCPValue val = cfg->value(YCPString(key));
	if (val->isBoolean())
	{
	    const bool prefer_download = val->asBoolean()->value();
	    y2milestone("new download_media_prefer_download value: %s", prefer_download ? "true" : "false");
	    zconfig.set_download_media_prefer_download(prefer_download);
	}
	else
	{
	    y2error("Expected boolean value for '%s' key, found %s", key, val->toString().c_str());
	    return YCPBoolean(false);
	}
    }

    key = "update_messages_notify";
    if(!cfg->value(YCPString(key)).isNull())
    {
	const YCPValue val = cfg->value(YCPString(key));
	if (val->isString())
	{
	    const std::string notify(val->asString()->value());
	    y2milestone("new update_messages_notify value: %s", notify.c_str());
	    zconfig.setUpdateMessagesNotify(notify);
	}
	else
	{
	    y2error("Expected string value for '%s' key, found %s", key, val->toString().c_str());
	    return YCPBoolean(false);
	}
    }

    key = "solver_upgrade_remove_dropped_packages";
    if(!cfg->value(YCPString(key)).isNull())
    {
	const YCPValue val = cfg->value(YCPString(key));
	if (val->isBoolean())
	{
	    const bool drop_packages = val->asBoolean()->value();
	    y2milestone("new solver_upgrade_remove_dropped_packages value: %s", drop_packages ? "true" : "false");
	    zconfig.setSolverUpgradeRemoveDroppedPackages(drop_packages);
	}
	else
	{
	    y2error("Expected boolean value for '%s' key, found %s", key, val->toString().c_str());
	    return YCPBoolean(false);
	}
    }

    return YCPBoolean(true);
}

bool PkgFunctions::RepoManagerUpdateTarget(const std::string& root, const YCPMap& options)
{
    bool new_target = _target_root != root;

    // a repository manager is present and the target has been changed
    // or the repo manager options changed
    if ((repo_manager && new_target) || options.compare(repo_options) != YO_EQUAL)
    {
        y2milestone("Updating RepoManager (target changed from %s to %s)", _target_root.c_str(), root.c_str());

        zypp::RepoManagerOptions repo_manager_options(root);

        y2debug("repomanager options size: %zd", options.size());
        if(!options->value(YCPString("target_distro")).isNull() && options->value(YCPString("target_distro"))->isString())
        {
            // override the target distribution autodetection
            y2milestone("Using target_distro: %s", options->value(YCPString("target_distro"))->asString()->value().c_str());
            repo_manager_options.servicesTargetDistro = options->value(YCPString("target_distro"))->asString()->value();
        }

        // repository manager options cannot be replaced, a new repository manager is needed
        zypp::RepoManager* new_repo_manager = new zypp::RepoManager(repo_manager_options);

        // register the known repositories
        if (repos.empty() && service_manager.empty())
        {
            for (RepoCont::iterator it = repos.begin();
                it != repos.end(); ++it)
            {
                // ignore removed repositories
                if (!(*it)->isDeleted())
                {
                    new_repo_manager->addRepository((*it)->repoInfo());
                }
            }
        }

        // replace the old repository manager
        if (repo_manager) delete repo_manager;
        repo_manager = new_repo_manager;

        // remember the repo options for the next time
        repo_options = options;
    }

    // update package cache path for loaded repositories when changing the target
    if (new_target)
    {
        zypp::RepoManagerOptions repo_options(root);
        zypp::Pathname packages_prefix = repo_options.repoPackagesCachePath;

        zypp::ResPool pool = zypp_ptr()->pool();
        for_(it, pool.knownRepositoriesBegin(), pool.knownRepositoriesEnd())
        {
            zypp::RepoInfo repo = it->info();
            repo.setPackagesPath(packages_prefix / repo.escaped_alias());

            y2milestone("Setting package cache for repository %s: %s",
                repo.alias().c_str(),
                repo.packagesPath().asString().c_str());

            it->setInfo(repo);
        }
    }

    return new_target;
}

bool PkgFunctions::SetTarget(const std::string &root, const YCPMap& options)
{
    bool new_target = RepoManagerUpdateTarget(root, options);
    _target_root = root;

    return new_target;
}


YCPValue
PkgFunctions::ExpandedName(const YCPString& name) const
{
    if (name.isNull())
    {
	y2error("name is nil");
	return YCPVoid();
    }

    return YCPString(ExpandedName(name->value()));
}


string PkgFunctions::ExpandedName(const string& name) const
{
    zypp::RepoVariablesReplacedString replaced_name;
    replaced_name.raw() = name;
    return replaced_name.transformed();
}


/**
 * @builtin ExpandedUrl
 * @short expands the repo variables in the given zypper URL
 * @return string copy of url with the variables expanded to the current value
 */
YCPValue PkgFunctions::ExpandedUrl(const YCPString &url)
{
    if (url.isNull())
    {
        y2error("URL is nil");
        return YCPVoid();
    }

    // return full URL including the password if present
    return YCPString(ExpandedUrl(zypp::Url(url->value())).asCompleteString());
}


zypp::Url PkgFunctions::ExpandedUrl(const zypp::Url& url) const
{
    zypp::RepoVariablesReplacedUrl replacedUrl;
    replacedUrl.raw() = url;
    return replacedUrl.transformed();
}


/**
 * @builtin CompareVersions
 * @short compares rpm version strings ([epoch:]version[-release])
 * @param ver1 First version to compare
 * @param ver2 Second version to compare
 * @return -1 if ver1 is older; 0 if they are equal; 1 if ver2 is older
 * @see https://github.com/openSUSE/zypper/blob/42fe3245f32d99ebb2bf643add2964de08fac0de/src/Zypper.cc#L5294
 */
YCPInteger PkgFunctions::CompareVersions(const YCPString& ver1, const YCPString& ver2) {
    zypp::Edition lhs(ver1->asString()->value());
    zypp::Edition rhs(ver2->asString()->value());

    return lhs.compare(rhs);
}
