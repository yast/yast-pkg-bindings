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
   Summary:     Handles Pkg::function (list_of_arguments) calls
   Namespace:   Pkg
*/

#ifndef PkgFunctions_h
#define PkgFunctions_h

#include <string>
#include <vector>

#include <ycp/YCPMap.h>

class YCPBoolean;
class YCPValue;
class YCPList;
class YCPSymbol;
class YCPString;
class YCPInteger;
class YCPVoid;
class YCPReference;

#include <zypp/ZYpp.h>
#include <zypp/Package.h>
#include <zypp/Product.h>

#include <zypp/DiskUsageCounter.h>
#include <zypp/RepoManager.h>
#include <zypp/ProgressData.h>
#include <zypp/TmpPath.h>
#include <zypp/ZYppCommitPolicy.h>

#include <YRepo.h>
#include <i18n.h>

#include "ServiceManager.h"
#include "BaseProduct.h"

#include "PkgError.h"
class PkgProgress;

namespace zypp
{
    class PoolQuery;
}

/**
 * A simple class for package management access
 */
class PkgFunctions
{
  public:

    /**
     * Handler for YCPCallbacks received or triggered.
     * Needs access to WFM.
     **/
    class CallbackHandler;

  protected:

	zypp::Pathname _target_root;
	bool _target_loaded;
	bool _source_loaded;

	zypp::ZYpp::Ptr zypp_pointer;

        // use a single RepoManager instance to avoid "unused" metadata cleanup
        // see https://bugzilla.novell.com/show_bug.cgi?id=802665#c27
        zypp::RepoManager* repo_manager;

	// remember the main locale (set by SetLocale) for SetAdditionalLocales,
	// add the main locale to the additional ones
	zypp::Locale preferred_locale;

	zypp::Pathname _download_area;

	/**
	 * ZYPP
	 */
	zypp::ZYpp::Ptr zypp_ptr();


	// container for the internal structure
	typedef std::vector<YRepo_Ptr> RepoCont;

    public:

	// ID type
	typedef long long RepoId;

        RepoId current_repo_id() const { return current_repo; }

    private: // source related

      // all known installation sources
      RepoCont repos;

      // table for converting libzypp source type to Yast type (for backward compatibility)
      std::map<std::string, std::string> type_conversion_table;

      // flag for skipping autorefresh
      volatile bool autorefresh_skipped;

      // flag
      RepoId current_repo;

      RepoId last_reported_repo;
      int last_reported_mediumnr;

      YCPValue SourceRefreshHelper(const YCPInteger &id, bool forced = false);
      YCPValue ServiceRefreshHelper(const YCPString &alias, bool forced = false);

      // helper for updating repository manager after changing the target root
      // return true if the target root has been changed
      bool RepoManagerUpdateTarget(const std::string& root, const YCPMap& options = YCPMap());

      // set new target directory
      bool SetTarget(const std::string &root, const YCPMap& options = YCPMap());

      // configured or default download area
      zypp::Pathname download_area_path();

      // helper - is the network running?
      bool NetworkDetected();

      // is the URL remote?
      bool remoteRepo(const zypp::Url &url);

      // conversion methods for type string between Yast and libzypp (for backward compatibility)
      std::string zypp2yastType(const zypp::repo::RepoType &type);
      std::string yast2zyppType(const std::string &type);

      // helper - create a directory if it doesn't exist
      bool CreateDir(const std::string &path);
      // helper - copy a file or directory
      bool CopyToDir(const std::string &source, const std::string &target, bool backup = false, bool recursive = true);

      void RemoveResolvablesFrom(YRepo_Ptr repo);
      bool LoadResolvablesFrom(YRepo_Ptr repo, const zypp::ProgressData::ReceiverFnc & progressrcv = zypp::ProgressData::ReceiverFnc(), bool network_check = false);
      std::string UniqueAlias(const std::string &alias);

      YCPValue GetPkgLocation(const YCPString& p, bool full_path);
      YCPValue PkgProp(const zypp::PoolItem &item);
      YCPValue PkgMediaSizesOrCount (bool sizes, bool download_size = false);
      YCPValue TargetInitInternal(const YCPString& root, bool rebuild_rpmdb);

      bool aliasExists(const std::string &alias, const std::list<zypp::RepoInfo> &reps) const;

      // remember the base product attributes for finding it later in
      // the installed system
      void RememberBaseProduct(const std::string &alias);

      zypp::RepoManager* CreateRepoManager();

      void SetCurrentDU();

      // callback related funcions
      void CallSourceReportStart(const std::string &text);
      void CallSourceReportEnd(const std::string &text);
      void CallSourceReportInit();
      void CallSourceReportDestroy();

      void CallInitDownload(const std::string &task);
      void CallDestDownload();
      // use "RefreshForced" by default, otherwise libzypp might download only the index file (bsc#1180203)
      void RefreshWithCallbacks(const zypp::RepoInfo &repo,
	const zypp::ProgressData::ReceiverFnc & progressrcv = zypp::ProgressData::ReceiverFnc(),
	zypp::RepoManager::RawMetadataRefreshPolicy refresh = zypp::RepoManager::RefreshForced);
      zypp::repo::RepoType ProbeWithCallbacks(const zypp::Url &url);
      void ScanProductsWithCallBacks(const zypp::Url &url);
      void CallRefreshStarted();
      void CallRefreshDone();
      YCPValue SourceProvideDirectoryInternal(const YCPInteger& id, const YCPInteger& mid,
	const YCPString& d, const YCPBoolean &optional,
	const YCPBoolean &recursive, bool check_signatures);

      YCPValue SourceLoadImpl(PkgProgress &progress);
      YCPValue SourceStartManagerImpl(const YCPBoolean& enable, PkgProgress &progress);

      // After all, APPL_HIGH might be more appropriate, because we suggest
      // the user what he should do and if it does not work, it's his job to
      // fix it (using USER). --ma
      static const zypp::ResStatus::TransactByValue whoWantsIt;	// #156875

      // convert MountPointSet to YCP Map
      YCPMap MPS2YCPMap(const zypp::DiskUsageCounter::MountPointSet &mps);

      zypp::Url shortenUrl(const zypp::Url &url);

      // convert Exception to string representation
      std::string ExceptionAsString(const zypp::Exception &e);

      YCPValue searchPackage(const YCPString &package, bool installed);

      bool CreateBaseProductSymlink();

      YCPMap Resolvable2YCPMap(const zypp::PoolItem &item, bool all, bool deps, const YCPList &attrs);

      // CommitPolicy used for commit
      zypp::ZYppCommitPolicy *commit_policy;

      // getPackageFromRepo used for PkgFunctions::ProvidePackage
      zypp::Package::constPtr packageFromRepo(const YCPInteger & repo_id, const YCPString & name);
    private:

      /**
       * Handler for YCPCallbacks received or triggered.
       * Needs access to WFM. See @ref CallbackHandler.
       **/
      CallbackHandler & _callbackHandler;

      PkgError _last_error;

      ServiceManager service_manager;

      BaseProduct* base_product;

      std::vector<zypp::filesystem::TmpDir> tmp_dirs;

      YCPMap repo_options;

      /**
       * Logging helper:
       * search for a repository and in case of exception, log error
       * and setLastError AND RETHROW
       */
	YRepo_Ptr logFindRepository(RepoId id);

	RepoId createManagedSource(const zypp::Url & url_r,
	    const zypp::Pathname & path_r, const std::string& type,
	    const std::string &alias_r, PkgProgress &progress, const zypp::ProgressData::ReceiverFnc & progressrcv = zypp::ProgressData::ReceiverFnc());

      /**
       * provides SourceProvideFile and SourceProvideFileCommon
       */
      YCPValue SourceProvideFileCommon(const YCPInteger &id, const YCPInteger &mid,
		const YCPString& f, const bool optional, const bool check_signatures,
		const bool digested);

      // a helper function to add or remove an upgrade repository
      YCPValue AddRemoveUpgradeRepo(const YCPInteger &repo, bool add);

      // action for a resolvable
      enum ResolvableAction
      {
        Install,
        Remove,
        Update
      };

      // helper for installing/removing/upgrading a resolvable
      bool ResolvableUpdateInstallOrDelete(const YCPString& name_r, const YCPSymbol& kind_r, ResolvableAction action);

      // it finds the resolvable using attributes saved earlier by RememberBaseProduct
      zypp::Product::constPtr FindInstalledBaseProduct();

      // helper with common code to SourceURL and SourceRawUrl
      YCPValue GetSourceUrl(const YCPInteger& id, bool raw);
      // helper - convert transaction_by to string
      std::string TransactToString(zypp::ResStatus::TransactByValue trans);

    public:
	// general
	/* TYPEINFO: void(string) */
	YCPValue SetTextLocale(const YCPString&);
	/* TYPEINFO: void(string) */
	YCPValue SetPackageLocale(const YCPString&);
	/* TYPEINFO: string() */
	YCPValue GetTextLocale();
	/* TYPEINFO: string() */
	YCPValue GetPackageLocale();
	/* TYPEINFO: void(list<string>) */
	YCPValue SetAdditionalLocales (const YCPList &args);
	/* TYPEINFO: list<string>() */
	YCPValue GetAdditionalLocales ();
	/* TYPEINFO: string() */
	YCPValue LastError ();
	/* TYPEINFO: string() */
	YCPValue LastErrorDetails ();
	/* TYPEINFO: boolean() */
	YCPValue Connect ();
	/* TYPEINFO: string(string)*/
	YCPValue ExpandedName(const YCPString&) const;
	/* TYPEINFO: string(string)*/
	YCPValue ExpandedUrl (const YCPString&);

	// callbacks
	/* TYPEINFO: void(void(string,integer,boolean)) */
	YCPValue CallbackStartProvide (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackProgressProvide (const YCPValue& /*nil*/ args);
	// FIXME: create ErrorProvide
	/* TYPEINFO: void(string(integer,string,string)) */
	YCPValue CallbackDoneProvide (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string,string,string,integer,boolean)) */
	YCPValue CallbackStartPackage (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackProgressPackage (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(string(integer,string)) */
	YCPValue CallbackDonePackage (const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void(string,integer)) */
	YCPValue CallbackStartDeltaDownload( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackProgressDeltaDownload( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string)) */
	YCPValue CallbackProblemDeltaDownload( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string)) */
	YCPValue CallbackStartDeltaApply( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer)) */
	YCPValue CallbackProgressDeltaApply( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string)) */
	YCPValue CallbackProblemDeltaApply( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackFinishDeltaDownload( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackPkgGpgCheck( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void()) */
	YCPValue CallbackFinishDeltaApply( const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void(string,string)) */
	YCPValue CallbackStartDownload (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer,integer,integer)) */
	YCPValue CallbackProgressDownload (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,string)) */
	YCPValue CallbackDoneDownload (const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void(string)) */
	YCPValue CallbackInitDownload( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void()) */
	YCPValue CallbackDestDownload( const YCPValue& /*nil*/ args );

	/* TYPEINFO: void(void(string)) */
	YCPValue CallbackSourceCreateStart( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackSourceCreateProgress( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(symbol(string,symbol,string)) */
	YCPValue CallbackSourceCreateError( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string,symbol,string)) */
	YCPValue CallbackSourceCreateEnd( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void()) */
	YCPValue CallbackSourceCreateInit( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void()) */
	YCPValue CallbackSourceCreateDestroy( const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void(string)) */
	YCPValue CallbackSourceProbeStart( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string,string)) */
	YCPValue CallbackSourceProbeFailed( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string,string)) */
	YCPValue CallbackSourceProbeSucceeded( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(string,symbol,string)) */
	YCPValue CallbackSourceProbeEnd( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(string,integer)) */
	YCPValue CallbackSourceProbeProgress( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(symbol(string,symbol,string)) */
	YCPValue CallbackSourceProbeError( const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void()) */
	YCPValue CallbackSourceReportInit( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void()) */
	YCPValue CallbackSourceReportDestroy( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,string,string)) */
	YCPValue CallbackSourceReportStart( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackSourceReportProgress( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(symbol(integer,string,symbol,string)) */
	YCPValue CallbackSourceReportError( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,string,string,symbol,string)) */
	YCPValue CallbackSourceReportEnd( const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void(integer,string,boolean,boolean,integer,integer,integer,integer)) */
	YCPValue CallbackProgressReportStart(const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(integer,integer,integer)) */
	YCPValue CallbackProgressReportProgress(const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer)) */
	YCPValue CallbackProgressReportEnd(const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void()) */
	YCPValue CallbackStartRefresh( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void()) */
	YCPValue CallbackDoneRefresh( const YCPValue& /*nil*/ args );

        /* TYPEINFO: void(void()) */
	YCPValue CallbackFileConflictStart( const YCPValue& args );
        /* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackFileConflictProgress( const YCPValue& args );
        /* TYPEINFO: void(boolean(list<string>,list<string>)) */
	YCPValue CallbackFileConflictReport( const YCPValue& args );
        /* TYPEINFO: void(void()) */
	YCPValue CallbackFileConflictFinish( const YCPValue& args );

	// Script (patch installation) callbacks
	/* TYPEINFO: void(void(string,string,string,string)) */
	YCPValue CallbackScriptStart( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(boolean,string)) */
	YCPValue CallbackScriptProgress( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(string(string)) */
	YCPValue CallbackScriptProblem( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void()) */
	YCPValue CallbackScriptFinish( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(string,string,string,string)) */
	YCPValue CallbackMessage( const YCPValue& /*nil*/ args );

	/* TYPEINFO: void(map<string,any>(string,string,string,string)) */
	YCPValue CallbackAuthentication( const YCPValue& /*nil*/ args );

	/* TYPEINFO: void(string(string,string,string,string,integer,string,integer,string,boolean,list<string>,integer)) */
	YCPValue CallbackMediaChange (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,integer)) */
	YCPValue CallbackSourceChange (const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void()) */
        YCPValue CallbackStartRebuildDb (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer)) */
        YCPValue CallbackProgressRebuildDb (const YCPValue& /*nil*/ args);
	// FIXME: not used
	/* TYPEINFO: void(void()) */
        YCPValue CallbackNotifyRebuildDb (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,string)) */
        YCPValue CallbackStopRebuildDb (const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void(string)) */
        YCPValue CallbackStartConvertDb (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,string)) */
        YCPValue CallbackProgressConvertDb (const YCPValue& /*nil*/ args);
	// FIXME: not used
	/* TYPEINFO: void(void()) */
        YCPValue CallbackNotifyConvertDb (const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(integer,string)) */
        YCPValue CallbackStopConvertDb (const YCPValue& /*nil*/ args);

	/* TYPEINFO: void(void()) */
	YCPValue CallbackStartScanDb( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackProgressScanDb( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(string(integer,string)) */
	YCPValue CallbackErrorScanDb( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void(integer,string)) */
	YCPValue CallbackDoneScanDb( const YCPValue& /*nil*/ args );


	/* TYPEINFO: void(void(string,string,string)) */
	YCPValue CallbackResolvableReport( const YCPValue& /*nil*/ args );

	/* TYPEINFO: void(boolean(map<string,any>,integer)) */
	YCPValue CallbackImportGpgKey( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(string,string,integer)) */
	YCPValue CallbackAcceptUnknownGpgKey( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(string,integer)) */
	YCPValue CallbackAcceptUnsignedFile( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(string,map<string,any>,integer)) */
	YCPValue CallbackAcceptVerificationFailed( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(string,string,string)) */
        YCPValue CallbackAcceptWrongDigest( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(boolean(string,string)) */
        YCPValue CallbackAcceptUnknownDigest( const YCPValue& /*nil*/ args);
	/* TYPEINFO: void(void(map<string,any>)) */
	YCPValue CallbackTrustedKeyAdded( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void(map<string,any>)) */
	YCPValue CallbackTrustedKeyRemoved( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(string)) */
	YCPValue CallbackAcceptFileWithoutChecksum( const YCPValue& /*nil*/ args );


	/* TYPEINFO: void(void(string,list<string>,string)) */
	YCPValue CallbackProcessStart( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(boolean(integer)) */
	YCPValue CallbackProcessProgress( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void()) */
	YCPValue CallbackProcessNextStage( const YCPValue& /*nil*/ args );
	/* TYPEINFO: void(void()) */
	YCPValue CallbackProcessDone( const YCPValue& /*nil*/ args );

	// source related
	/* TYPEINFO: boolean(boolean)*/
        YCPValue SourceStartManager (const YCPBoolean&);
	/* TYPEINFO: boolean()*/
	YCPValue SourceRestore();
	/* TYPEINFO: boolean()*/
	YCPValue SourceLoad();
	/* TYPEINFO: integer(string,string)*/
	YCPValue SourceCreate (const YCPString&, const YCPString&);
	/* TYPEINFO: integer(string,string)*/
	YCPValue SourceCreateBase (const YCPString&, const YCPString&);
	/* TYPEINFO: integer(string,string,string)*/
	YCPValue SourceCreateType (const YCPString& media, const YCPString& pd, const YCPString& type);
	YCPValue SourceCreateEx (const YCPString&, const YCPString&, bool, const YCPString& source_type, bool scan_only = false);
	/* TYPEINFO: list<integer>(boolean)*/
	YCPValue SourceStartCache (const YCPBoolean&);
	/* TYPEINFO: list<integer>(boolean)*/
	YCPValue SourceGetCurrent (const YCPBoolean& enabled);
	/* TYPEINFO: boolean()*/
	YCPValue SourceSaveAll ();
	/* TYPEINFO: boolean()*/
	YCPValue SourceFinishAll ();
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceGeneralData (const YCPInteger&);
	/* TYPEINFO: string(integer)*/
	YCPValue SourceURL (const YCPInteger&);
	/* TYPEINFO: string(integer)*/
	YCPValue SourceRawURL (const YCPInteger&);
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceMediaData (const YCPInteger&);
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceProductData (const YCPInteger&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideFile (const YCPInteger&, const YCPInteger&, const YCPString&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideOptionalFile (const YCPInteger&, const YCPInteger&, const YCPString&);
	/* TYPEINFO: string(integer,integer,string,boolean,boolean)*/
	YCPValue SourceProvideDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive);
	/* TYPEINFO: string(integer,integer,string,boolean,boolean)*/
	YCPValue SourceProvideSignedDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive);
	/* TYPEINFO: string(integer,integer,string,boolean)*/
	YCPValue SourceProvideSignedFile(const YCPInteger& id, const YCPInteger& mid, const YCPString& f, const YCPBoolean &optional);
	/* TYPEINFO: string(integer,integer,string,boolean)*/
	YCPValue SourceProvideDigestedFile(const YCPInteger& id, const YCPInteger& mid, const YCPString& f, const YCPBoolean &optional);
	/* TYPEINFO: boolean(string)*/
	YCPValue SourceCacheCopyTo (const YCPString&);
	/* TYPEINFO: boolean(integer,boolean)*/
        YCPValue SourceSetEnabled (const YCPInteger&, const YCPBoolean&);
    /* TYPEINFO: boolean(integer,integer) */
    YCPValue SourceSetPriority(const YCPInteger& id, const YCPInteger& priority);
	/* TYPEINFO: boolean(integer,boolean)*/
        YCPValue SourceSetAutorefresh (const YCPInteger&, const YCPBoolean&);
	/* TYPEINFO: boolean(integer)*/
        YCPValue SourceRefreshNow (const YCPInteger&);
	/* TYPEINFO: boolean(integer)*/
        YCPValue SourceForceRefreshNow (const YCPInteger&);
	/* TYPEINFO: boolean(integer)*/
        YCPValue SourceDelete (const YCPInteger&);
	/* TYPEINFO: void(integer)*/
        YCPValue SourceRaisePriority (const YCPInteger&);
	/* TYPEINFO: void(integer)*/
        YCPValue SourceLowerPriority (const YCPInteger&);
	/* TYPEINFO: boolean(integer,string)*/
        YCPValue SourceChangeUrl (const YCPInteger&, const YCPString&);
	/* TYPEINFO: list<map<string,any>>()*/
        YCPValue SourceEditGet ();
	/* TYPEINFO: boolean(list<map<string,any>>)*/
        YCPValue SourceEditSet (const YCPList& args);
	/* TYPEINFO: list<integer>(string,string)*/
        YCPValue SourceScan (const YCPString& media, const YCPString& product_dir);
	/* TYPEINFO: string(string,string)*/
	YCPValue RepositoryProbe(const YCPString& url, const YCPString& prod_dir);
	/* TYPEINFO: list<list<string> >(string)*/
	YCPValue RepositoryScan(const YCPString& url);
	/* TYPEINFO: integer(map<string,any>)*/
	YCPValue RepositoryAdd(const YCPMap &params);
	/* TYPEINFO: void()*/
	YCPValue SkipRefresh();

	// target related
	/* TYPEINFO: boolean(string,boolean)*/
	YCPValue TargetInit (const YCPString& root, const YCPBoolean& unused);
        /* TYPEINFO: boolean(string)*/
        YCPValue TargetInitialize (const YCPString& root);
        /* TYPEINFO: boolean(string, map<any,any>)*/
        YCPValue TargetInitializeOptions (const YCPString& root, const YCPMap& options);
        /* TYPEINFO: boolean()*/
        YCPValue TargetLoad ();
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetDisableSources ();
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetFinish ();
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetCapacity (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetUsed (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetAvailable (const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetInstall (const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetRemove (const YCPString&);
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetRebuildDB ();
	/* TYPEINFO: void(list<map<any,any>>)*/
	YCPValue TargetInitDU (const YCPList&);
	/* TYPEINFO: map<string,list<integer>>()*/
	YCPValue TargetGetDU ();
	/* TYPEINFO: boolean(string,symbol)*/
	YCPBoolean TargetStoreRemove(const YCPString& root, const YCPSymbol& kind_r);

	// backup related
	/* TYPEINFO: string()*/
	YCPValue GetBackupPath ();
	/* TYPEINFO: void(string)*/
	YCPValue SetBackupPath (const YCPString& path);
	/* TYPEINFO: void(boolean)*/
	YCPValue CreateBackups (const YCPBoolean& flag);

	// package related
	/* TYPEINFO: list<string>(symbol,boolean)*/
	YCPValue GetPackages (const YCPSymbol& which, const YCPBoolean& names_only);
	/* TYPEINFO: list<string>(boolean,boolean,boolean,boolean)*/
	YCPValue FilterPackages (const YCPBoolean& byAuto, const YCPBoolean& byApp, const YCPBoolean& byUser, const YCPBoolean& names_only);
	/* TYPEINFO: boolean(string)*/
	YCPValue IsProvided (const YCPString& tag);
	/* TYPEINFO: boolean(string)*/
	YCPValue PkgInstalled(const YCPString& package);
	/* TYPEINFO: boolean(string)*/
	YCPValue IsSelected (const YCPString& tag);
	/* TYPEINFO: boolean(string)*/
	YCPValue IsAvailable (const YCPString& tag);
	/* TYPEINFO: boolean(string)*/
	YCPValue PkgAvailable(const YCPString& package);
	/* TYPEINFO: map<string,any>(list<string>)*/
	YCPValue DoProvide (const YCPList& args);
	/* TYPEINFO: map<string,any>(list<string>)*/
	YCPValue DoRemove (const YCPList& args);
	/* TYPEINFO: string(string)*/
	YCPValue PkgSummary (const YCPString& package);
	/* TYPEINFO: string(string)*/
	YCPValue PkgVersion (const YCPString& package);
	/* TYPEINFO: integer(string)*/
	YCPValue PkgSize (const YCPString& package);
	/* TYPEINFO: string(string)*/
	YCPValue PkgGroup (const YCPString& package);
	/* TYPEINFO: string(string)*/
	YCPValue PkgLocation (const YCPString& package);
	/* TYPEINFO: string(string)*/
	YCPValue PkgPath (const YCPString& package);
	/* TYPEINFO: map<string,any>(string)*/
	YCPValue PkgProperties (const YCPString& package);
	/* TYPEINFO: list<map<string,any> >(string)*/
	YCPValue PkgPropertiesAll (const YCPString& package);
	/* TYPEINFO: list<string>(string,symbol)*/
	YCPList  PkgGetFilelist (const YCPString& package, const YCPSymbol& which);
	/* TYPEINFO: map<string,list<integer>>(string)*/
	YCPValue PkgDU(const YCPString& package);
	/* TYPEINFO: boolean()*/
	YCPValue IsManualSelection ();
	/* TYPEINFO: boolean()*/
	YCPValue ClearSaveState ();
	/* TYPEINFO: boolean()*/
	YCPValue SaveState ();
	/* TYPEINFO: boolean(boolean)*/
	YCPValue RestoreState (const YCPBoolean&);
	/* TYPEINFO: map<symbol,integer>(map<string,any>)*/
	YCPValue PkgUpdateAll (const YCPMap& options);
	/* TYPEINFO: list<list<any>>(string) */
	YCPList  PkgQueryProvides(const YCPString& tag);

	/* TYPEINFO: boolean(string)*/
	YCPValue PkgInstall (const YCPString& p);
	/* TYPEINFO: boolean(string)*/
	YCPValue PkgSrcInstall (const YCPString& p);
	/* TYPEINFO: boolean(string)*/
	YCPValue PkgDelete (const YCPString& p);
	/* TYPEINFO: boolean(string)*/
	YCPValue PkgTaboo (const YCPString& p);
	/* TYPEINFO: boolean(string)*/
	YCPValue PkgNeutral (const YCPString& p);
	/* TYPEINFO: boolean()*/
	YCPValue PkgReset ();
	/* TYPEINFO: boolean()*/
	YCPValue PkgApplReset ();
        /* TYPEINFO: boolean(integer,string,string) */
	YCPValue ProvidePackage(const YCPInteger & repo_id, const YCPString & name, const YCPString & path);
	/* TYPEINFO: map<string,any>()*/
	YCPValue GetSolverFlags();
	/* TYPEINFO: boolean(map<string,any>)*/
	YCPValue SetSolverFlags(const YCPMap& params);
	/* TYPEINFO: boolean(list<string>) */
	YCPValue SetAdditionalVendors (const YCPList &args);
	/* TYPEINFO: boolean(boolean)*/
	YCPBoolean PkgSolve (const YCPBoolean& filter);
	/* TYPEINFO: boolean(string)*/
	YCPValue CreateSolverTestCase(const YCPString &dir);
	/* TYPEINFO: boolean()*/
	YCPBoolean PkgSolveCheckTargetOnly ();
	/* TYPEINFO: integer()*/
	YCPValue PkgSolveErrors ();
	/* TYPEINFO: list<map<string,any>>()*/
	YCPValue PkgSolveProblems ();
	/* TYPEINFO: boolean(list<map<string,any>>)*/
	YCPValue PkgSetSolveSolutions (const YCPList& solutions);
	/* TYPEINFO: void()*/
	YCPValue PkgResetSolveSolutions ();
        YCPValue CommitHelper(const zypp::ZYppCommitPolicy *policy);
	/* TYPEINFO: list<any>(integer)*/
	YCPValue PkgCommit (const YCPInteger& medianr);
	/* TYPEINFO: list<any>(map<string,any>)*/
	YCPValue Commit (const YCPMap& config);
	/* TYPEINFO: map<string,any>()*/
	YCPValue CommitPolicy();
	/* TYPEINFO: boolean(integer)*/
	YCPValue AddUpgradeRepo(const YCPInteger &repo);
	/* TYPEINFO: list<integer>()*/
	YCPValue GetUpgradeRepos();
	/* TYPEINFO: boolean(integer)*/
	YCPValue RemoveUpgradeRepo(const YCPInteger &repo);

	/* TYPEINFO: list<list<integer>>()*/
	YCPValue PkgMediaSizes ();
	/* TYPEINFO: list<list<integer>>()*/
	YCPValue PkgMediaPackageSizes();
	/* TYPEINFO: list<list<integer>>()*/
	YCPValue PkgMediaCount();
	/* TYPEINFO: list<list<any>>()*/
	YCPValue PkgMediaNames ();

	/* TYPEINFO: string(string)*/
	YCPString PkgGetLicenseToConfirm( const YCPString & package );
	/* TYPEINFO: map<string,string>(list<string>)*/
	YCPMap    PkgGetLicensesToConfirm( const YCPList & packages );
	/* TYPEINFO: string(string)*/
	YCPBoolean PkgMarkLicenseConfirmed (const YCPString & package);
	/* TYPEINFO: string(string)*/
	YCPValue PrdGetLicenseToConfirm (const YCPString& product, const YCPString& localeCode);
	/* TYPEINFO: boolean(string)*/
	YCPValue PrdMarkLicenseConfirmed (const YCPString& product);
	/* TYPEINFO: boolean(string)*/
	YCPValue PrdMarkLicenseNotConfirmed(const YCPString& product);
	/* TYPEINFO: boolean(string)*/
	YCPValue PrdNeedToAcceptLicense (const YCPString& product);
	/* TYPEINFO: boolean(string)*/
	YCPValue PrdHasLicenseConfirmed(const YCPString& product);
	/* TYPEINFO: list<string>(string)*/
	YCPValue PrdLicenseLocales(const YCPString& product);

	/* TYPEINFO: boolean(string)*/
	YCPBoolean RpmChecksig( const YCPString & filename );

	// architecture related
	/* TYPEINFO: string()*/
	YCPValue GetArchitecture();

	/* TYPEINFO: boolean(string,symbol)*/
        YCPValue ResolvableInstall( const YCPString& name_r, const YCPSymbol& kind_r );
	/* TYPEINFO: boolean(string,symbol,string,string)*/
	YCPValue ResolvableInstallArchVersion( const YCPString& name_r, const YCPSymbol& kind_r, const YCPString& arch, const YCPString& vers );
	/* TYPEINFO: boolean(string,symbol,integer)*/
	YCPValue ResolvableInstallRepo( const YCPString& name_r, const YCPSymbol& kind_r, const YCPInteger& repo_r );
	/* TYPEINFO: boolean(string,symbol)*/
        YCPValue ResolvableUpdate( const YCPString& name_r, const YCPSymbol& kind_r );
	/* TYPEINFO: boolean(string,symbol)*/
        YCPValue ResolvableRemove( const YCPString& name_r, const YCPSymbol& kind_r );
	/* TYPEINFO: boolean(string,symbol,boolean)*/
        YCPValue ResolvableNeutral( const YCPString& name_r, const YCPSymbol& kind_r, const YCPBoolean& force_r );
	/* TYPEINFO: boolean(string,symbol)*/
        YCPValue ResolvableSetSoftLock( const YCPString& name_r, const YCPSymbol& kind_r );
	/* TYPEINFO: list<map<string,any> >(string,symbol,string)*/
        YCPValue ResolvableProperties(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version);
	/* TYPEINFO: list<map<string,any> >(string,symbol,string)*/
        YCPValue ResolvableDependencies(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version);
	/* TYPEINFO: integer(symbol)*/
	YCPValue ResolvablePreselectPatches(const YCPSymbol& kind_r);
	/* TYPEINFO: integer(symbol)*/
	YCPValue ResolvableCountPatches(const YCPSymbol& kind_r);
	/* TYPEINFO: boolean(symbol,symbol)*/
	YCPValue IsAnyResolvable(const YCPSymbol& kind_r, const YCPSymbol& status);

	/* TYPEINFO: list<map<string,any> >(map<symbol,any>, list<symbol>) */
	YCPValue Resolvables(const YCPMap& filter, const YCPList& attrs);
	/* TYPEINFO: boolean(map<symbol,any>) */
	YCPValue AnyResolvable(const YCPMap& filter);

	// keyring related
	/* TYPEINFO: boolean(string,boolean)*/
	YCPValue ImportGPGKey(const YCPString& filename, const YCPBoolean& trusted);
	/* TYPEINFO: list<map<string,any>>(boolean)*/
	YCPValue GPGKeys(const YCPBoolean& trusted);
	/* TYPEINFO: boolean(string,boolean)*/
	YCPValue DeleteGPGKey(const YCPString&, const YCPBoolean&);
	/* TYPEINFO: map<string,any>(string)*/
	YCPValue CheckGPGKeyFile(const YCPString&);

	/* TYPEINFO: boolean()*/
	YCPValue SourceReleaseAll ();
	/* TYPEINFO: boolean(string)*/
	YCPValue SourceMoveDownloadArea (const YCPString & path);

	// services related functions
	/* TYPEINFO: list<string>()*/
	YCPValue ServiceAliases();
	/* TYPEINFO: boolean(string,string)*/
	YCPValue ServiceAdd(const YCPString&, const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPValue ServiceDelete(const YCPString&);
	/* TYPEINFO: map<string,any>(string)*/
	YCPValue ServiceGet(const YCPString&);
	/* TYPEINFO: boolean(string,map<string,any>)*/
	YCPValue ServiceSet(const YCPString&, const YCPMap&);
	/* TYPEINFO: boolean(string)*/
	YCPValue ServiceRefresh(const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPValue ServiceForceRefresh(const YCPString&);
	/* TYPEINFO: string(string)*/
	YCPValue ServiceURL(const YCPString &alias);
	/* TYPEINFO: string(string)*/
	YCPValue ServiceProbe(const YCPString &url);
	/* TYPEINFO: boolean(string)*/
	YCPValue ServiceSave(const YCPString &alias);

        // configuration related functions
	/* TYPEINFO: map<string,any>()*/
	YCPValue ZConfig();
	/* TYPEINFO: boolean(map<string,any>)*/
	YCPValue SetZConfig(const YCPMap &cfg);

    // URL related functions
    /* TYPEINFO: list<string>() */
    YCPValue UrlKnownSchemes();
    /* TYPEINFO: boolean(string) */
    YCPValue UrlSchemeIsRemote(const YCPString &url_scheme);
    /* TYPEINFO: boolean(string) */
    YCPValue UrlSchemeIsLocal(const YCPString &url_scheme);
    /* TYPEINFO: boolean(string) */
    YCPValue UrlSchemeIsVolatile(const YCPString &url_scheme);
    /* TYPEINFO: boolean(string) */
    YCPValue UrlSchemeIsDownloading(const YCPString &url_scheme);

	YCPValue ResolvablePropertiesEx(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version, bool all, bool deps, const YCPList &attrs);
	YCPValue ResolvableSetPatches(const YCPSymbol& kind_r, bool preselect);

  /* TYPEINFO: integer(string, string) */
  YCPInteger CompareVersions(const YCPString& ver1, const YCPString& ver2);

	/**
	 * Constructor.
	 */
	PkgFunctions ();

	/**
	 * Destructor.
	 */
	virtual ~PkgFunctions ();

	// must be public, used in callbacks
	RepoId logFindAlias(const std::string &alias) const;

	RepoId LastReportedRepo() const;
	int LastReportedMedium() const;
	void SetReportedSource(RepoId repo, int medium);

    string ExpandedName(const string&) const;
    zypp::Url ExpandedUrl(const zypp::Url&) const;

};
#endif // PkgFunctions_h
