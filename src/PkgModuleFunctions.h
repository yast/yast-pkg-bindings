/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:	PkgModuleFunctions.h

   Author:	Klaus Kaempf <kkaempf@suse.de>
		Stanislav Visnovsky <visnov@suse.cz>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>

   Purpose:	Access to packagemanager
		Handles Pkg::function (list_of_arguments) calls
/-*/

#ifndef PkgModuleFunctions_h
#define PkgModuleFunctions_h

#include <string>
#include <ycpTools.h>

#include <ycp/YCPBoolean.h>
#include <ycp/YCPValue.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPVoid.h>
#include <ycp/YBlock.h>

#include <y2/Y2Namespace.h>

#include <zypp/ZYpp.h>
#include <zypp/base/PtrTypes.h>
#include <zypp/Pathname.h>
#include <zypp/Url.h>
#include <zypp/Arch.h>
#include <zypp/DiskUsageCounter.h>
#include <zypp/RepoInfo.h>
#include <zypp/RepoManager.h>
#include <zypp/MediaSetAccess.h>

#include "PkgError.h"


// textdomain
extern "C" {
#include <libintl.h>
}

// undefine _ macro from libzypp
#ifdef _
#undef _
#endif

// define new _ macro
#define _(MSG) ::dgettext("pkg-bindings", MSG)

DEFINE_PTR_TYPE(YRepo);
class YRepo : public zypp::base::ReferenceCounted
{
private:
    zypp::RepoInfo _repo;
    zypp::MediaSetAccess_Ptr _maccess;
    bool _deleted;

    YRepo() {}

public:
    YRepo(zypp::RepoInfo & repo);
    ~YRepo();

    const zypp::RepoInfo & repoInfo() const { return _repo; }
    zypp::RepoInfo & repoInfo() { return _repo; }
    zypp::MediaSetAccess_Ptr & mediaAccess();

    bool isDeleted() {return _deleted;}
    void setDeleted() {_deleted = true;}

public:
    static const YRepo NOREPO;
};

/**
 * A simple class for package management access
 */
class PkgModuleFunctions : public Y2Namespace
{
  public:

    /**
     * Handler for YCPCallbacks received or triggered.
     * Needs access to WFM.
     **/
    class CallbackHandler;

  protected:

	zypp::Pathname _target_root;
	
	zypp::ZYpp::Ptr zypp_pointer;

	// remember the main locale (set by SetLocale) for SetAdditionalLocales,
	// add the main locale to the additional ones
	zypp::Locale preferred_locale;

	/** 
	 * ZYPP
	 */
	zypp::ZYpp::Ptr zypp_ptr();


    private: // source related
    
      // all known installation sources
      std::vector<YRepo_Ptr> repos;

      // table for converting libzypp source type to Yast type (for backward compatibility)
      std::map<std::string, std::string> type_conversion_table;

      // conversion methods for type string between Yast and libzypp (for backward compatibility)
      std::string zypp2yastType(const std::string &type);
      std::string yast2zyppType(const std::string &type);

      bool DoProvideNameKind( const std::string & name, zypp::Resolvable::Kind kind, zypp::Arch architecture,
			      const std::string& version, const bool onlyNeeded = false);
      bool DoRemoveNameKind( const std::string & name, zypp::Resolvable::Kind kind);
      bool DoProvideAllKind(zypp::Resolvable::Kind kind);
      bool DoRemoveAllKind(zypp::Resolvable::Kind kind);
      bool DoAllKind(zypp::Resolvable::Kind kind, bool provide);

      void RemoveResolvablesFrom(const std::string &alias);
      bool AnyResolvableFrom(const std::string &alias);
      bool LoadResolvablesFrom(const zypp::RepoInfo &repoinfo);
      std::string UniqueAlias(const std::string &alias);

      YCPValue GetPkgLocation(const YCPString& p, bool full_path);
      YCPValue PkgProp( zypp::PoolItem_Ref item );
      YCPValue PkgMediaSizesOrCount (bool sizes);
    
      bool aliasExists(const std::string &alias) const;

      zypp::RepoManager CreateRepoManager();

      void SetCurrentDU();

      // builtin handling
      void registerFunctions ();
      vector<string> _registered_functions;

      // callback related funcions
      void CallSourceReportStart(const std::string &text);
      void CallSourceReportEnd(const std::string &text);
      void CallSourceReportInit();
      void CallSourceReportDestroy();

      void CallInitDownload(const std::string &task);
      void CallDestDownload();
      void RefreshWithCallbacks(const zypp::RepoInfo &repo);
      zypp::repo::RepoType ProbeWithCallbacks(const zypp::Url &url);
      void ScanProductsWithCallBacks(const zypp::Url &url);

      // After all, APPL_HIGH might be more appropriate, because we suggest
      // the user what he should do and if it does not work, it's his job to
      // fix it (using USER). --ma
      static const zypp::ResStatus::TransactByValue whoWantsIt;	// #156875

      // convert MountPointSet to YCP Map
      YCPMap MPS2YCPMap(const zypp::DiskUsageCounter::MountPointSet &mps);

      zypp::Url shortenUrl(const zypp::Url &url);

      // convert Exception to string represenatation
      std::string ExceptionAsString(const zypp::Exception &e);

    private:

      /**
       * Handler for YCPCallbacks received or triggered.
       * Needs access to WFM. See @ref CallbackHandler.
       **/
      CallbackHandler & _callbackHandler;

      PkgError _last_error;

      /**
       * Logging helper:
       * search for a repository and in case of exception, log error
       * and setLastError AND RETHROW
       */
	YRepo_Ptr logFindRepository(std::vector<YRepo_Ptr>::size_type id);
	
	std::vector<YRepo_Ptr>::size_type createManagedSource(const zypp::Url & url_r,
	    const zypp::Pathname & path_r, const bool base_source, const std::string& type,
	    const std::string &alias_r);

      /**
       * provides SourceProvideFile and SourceProvideFileCommon
       */
      YCPValue SourceProvideFileCommon (const YCPInteger&, const YCPInteger&, const YCPString&, const YCPBoolean&);
    public:
	// general
	/* TYPEINFO: void() */
	YCPValue InstSysMode ();
	/* TYPEINFO: void(string) */
	YCPValue SetLocale (const YCPString& locale);
	/* TYPEINFO: string() */
	YCPValue GetLocale ();
	/* TYPEINFO: void(string) */
	YCPValue SetTextLocale(const YCPString&);
	/* TYPEINFO: void(string) */
	YCPValue SetPackageLocale(const YCPString&);
	/* TYPEINFO: string() */
	YCPValue GetTextLocale();
	/* TYPEINFO: string() */
	YCPValue GetPackageLocale();
	/* TYPEINFO: void(list<string>) */
	YCPValue SetAdditionalLocales (YCPList args);
	/* TYPEINFO: list<string>() */
	YCPValue GetAdditionalLocales ();
	/* TYPEINFO: string() */
	YCPValue LastError ();
	/* TYPEINFO: string() */
	YCPValue LastErrorDetails ();
	/* TYPEINFO: string() */
	YCPValue LastErrorId ();
	/* TYPEINFO: boolean() */
	YCPValue Init ();
	/* TYPEINFO: boolean() */
	YCPValue Connect ();

	// callbacks
	/* TYPEINFO: void(string) */
	YCPValue CallbackStartProvide (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressProvide (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackDoneProvide (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackStartPackage (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressPackage (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackDonePackage (const YCPString& func);

	/* TYPEINFO: void(string) */
	YCPValue CallbackStartDeltaDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressDeltaDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProblemDeltaDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackStartDeltaApply( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressDeltaApply( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProblemDeltaApply( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackStartPatchDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressPatchDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProblemPatchDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackFinishDeltaDownload( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackFinishDeltaApply( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackFinishPatchDownload( const YCPString& func);

	/* TYPEINFO: void(string) */
	YCPValue CallbackStartDownload (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressDownload (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackDoneDownload (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackInitDownload( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackDestDownload( const YCPString& args );

	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceCreateStart( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceCreateProgress( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceCreateError( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceCreateEnd( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceCreateInit( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceCreateDestroy( const YCPString& func);

	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceProbeStart( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceProbeFailed( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceProbeSucceeded( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceProbeEnd( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceProbeProgress( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceProbeError( const YCPString& func);

	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceReportInit( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceReportDestroy( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceReportStart( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceReportProgress( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceReportError( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceReportEnd( const YCPString& func);

	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressReportStart(const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressReportProgress(const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressReportEnd(const YCPString& func);

	// Script (patch installation) callbacks
	/* TYPEINFO: void(string) */
	YCPValue CallbackScriptStart( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackScriptProgress( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackScriptProblem( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackScriptFinish( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackMessage( const YCPString& args );

	/* TYPEINFO: void(string) */
	YCPValue CallbackAuthentication( const YCPString& func );

	/* TYPEINFO: void(string) */
	YCPValue CallbackMediaChange (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackSourceChange (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackYouProgress (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackYouPatchProgress (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackYouError (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackYouMessage (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackYouLog (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackYouExecuteYcpScript (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackYouScriptProgress (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackStartRebuildDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackProgressRebuildDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackNotifyRebuildDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackStopRebuildDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackStartConvertDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackProgressConvertDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackNotifyConvertDb (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackStopConvertDb (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackStartScanDb( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressScanDb( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackErrorScanDb( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackDoneScanDb( const YCPString& args );


	/* TYPEINFO: void(string) */
	YCPValue CallbackResolvableReport( const YCPString& func );
	/* TYPEINFO: void(string) */
	YCPValue CallbackImportGpgKey( const YCPString& func );	
	/* TYPEINFO: void(string) */
	YCPValue CallbackAcceptNonTrustedGpgKey( const YCPString& args );
	/* TYPEINFO: void(string) */
	YCPValue CallbackAcceptUnknownGpgKey( const YCPString& func );	
	/* TYPEINFO: void(string) */
	YCPValue CallbackAcceptUnsignedFile( const YCPString& func );	
	/* TYPEINFO: void(string) */
	YCPValue CallbackAcceptVerificationFailed( const YCPString& func );	
	/* TYPEINFO: void(string) */
        YCPValue CallbackAcceptWrongDigest( const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackAcceptUnknownDigest( const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackTrustedKeyAdded( const YCPString& func );	
	/* TYPEINFO: void(string) */
	YCPValue CallbackTrustedKeyRemoved( const YCPString& func );	
	/* TYPEINFO: void(string) */
	YCPValue CallbackAcceptFileWithoutChecksum( const YCPString& func );	

	// source related
	/* TYPEINFO: boolean(boolean)*/
        YCPValue SourceStartManager (const YCPBoolean&);
	/* TYPEINFO: boolean()*/
	YCPValue SourceRestore();
	/* TYPEINFO: list<string>()*/
	YCPValue SourceGetBrokenSources();
	/* TYPEINFO: boolean()*/
	YCPValue SourceLoad();
	/* TYPEINFO: integer(string,string)*/
	YCPValue SourceCreate (const YCPString&, const YCPString&);
	/* TYPEINFO: integer(string,string)*/
	YCPValue SourceCreateBase (const YCPString&, const YCPString&);
	/* TYPEINFO: integer(string,string,string)*/
	YCPValue SourceCreateType (const YCPString& media, const YCPString& pd, const YCPString& type);
	YCPValue SourceCreateEx (const YCPString&, const YCPString&, bool, const YCPString& source_type);
	/* TYPEINFO: list<integer>(boolean)*/
	YCPValue SourceStartCache (const YCPBoolean&);
	/* TYPEINFO: boolean()*/
	YCPValue SourceCleanupBroken ();
	/* TYPEINFO: list<integer>(boolean)*/
	YCPValue SourceGetCurrent (const YCPBoolean& enabled);
	/* TYPEINFO: boolean(integer)*/
	YCPValue SourceFinish (const YCPInteger&);
	/* TYPEINFO: boolean()*/
	YCPValue SourceSaveAll ();
	/* TYPEINFO: boolean()*/
	YCPValue SourceFinishAll ();
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceGeneralData (const YCPInteger&);
	/* TYPEINFO: string(integer)*/
	YCPValue SourceURL (const YCPInteger&);
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceMediaData (const YCPInteger&);
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceProductData (const YCPInteger&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideFile (const YCPInteger&, const YCPInteger&, const YCPString&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideOptionalFile (const YCPInteger&, const YCPInteger&, const YCPString&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideDir (const YCPInteger&, const YCPInteger&, const YCPString&);
	/* TYPEINFO: string(integer,integer,string,boolean,boolean)*/
	YCPValue SourceProvideDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive);
	/* TYPEINFO: boolean(string)*/
	YCPValue SourceCacheCopyTo (const YCPString&);
	/* TYPEINFO: boolean(boolean)*/
	YCPValue SourceSetRamCache (const YCPBoolean&);
	/* TYPEINFO: map<string,string>(integer)*/
	YCPValue SourceProduct (const YCPInteger&);
	/* TYPEINFO: boolean(integer,boolean)*/
        YCPValue SourceSetEnabled (const YCPInteger&, const YCPBoolean&);
	/* TYPEINFO: boolean(integer,boolean)*/
        YCPValue SourceSetAutorefresh (const YCPInteger&, const YCPBoolean&);
	/* TYPEINFO: boolean(integer)*/
        YCPValue SourceRefreshNow (const YCPInteger&);
	/* TYPEINFO: boolean(integer)*/
        YCPValue SourceDelete (const YCPInteger&);
	/* TYPEINFO: void(integer)*/
        YCPValue SourceRaisePriority (const YCPInteger&);
	/* TYPEINFO: void(integer)*/
        YCPValue SourceLowerPriority (const YCPInteger&);
	/* TYPEINFO: boolean()*/
        YCPValue SourceSaveRanks ();
	/* TYPEINFO: boolean(integer,string)*/
        YCPValue SourceChangeUrl (const YCPInteger&, const YCPString&);
	/* TYPEINFO: boolean(map<integer,integer>)*/
	YCPValue SourceInstallOrder (const YCPMap&);
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

	// target related
	/* TYPEINFO: boolean(string,boolean)*/
	YCPValue TargetInit (const YCPString& root, const YCPBoolean& unused);
        /* TYPEINFO: boolean(string)*/
        YCPValue TargetInitialize (const YCPString& root);
        /* TYPEINFO: boolean()*/
        YCPValue TargetLoad ();
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetDisableSources ();
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetFinish ();
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetLogfile (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetCapacity (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetUsed (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetAvailable (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetBlockSize (const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetInstall (const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetRemove (const YCPString&);
	/* TYPEINFO: list<any>()*/
	YCPList TargetProducts ();
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetRebuildDB ();
	/* TYPEINFO: void(list<map<any,any>>)*/
	YCPValue TargetInitDU (const YCPList&);
	/* TYPEINFO: map<string,list<integer>>()*/
	YCPValue TargetGetDU ();
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetFileHasOwner (const YCPString&);
	/* TYPEINFO: boolean(string,symbol)*/
	YCPBoolean TargetStoreRemove(const YCPString& root, const YCPSymbol& kind_r);

	// selection related
	/* TYPEINFO: list<string>(symbol,string)*/
	YCPValue GetSelections (const YCPSymbol& stat, const YCPString& cat);
	/* TYPEINFO: list<string>(symbol,string)*/
	YCPValue GetPatterns (const YCPSymbol& stat, const YCPString& cat);
	/* TYPEINFO: string()*/
	YCPValue GetBackupPath ();
	/* TYPEINFO: void(string)*/
	YCPValue SetBackupPath (const YCPString& path);
	/* TYPEINFO: void(boolean)*/
	YCPValue CreateBackups (const YCPBoolean& flag);
	/* TYPEINFO: map<string,any>(string)*/
	YCPValue SelectionData (const YCPString& cat);
	/* TYPEINFO: map<string,any>(string)*/
	YCPValue PatternData (const YCPString& pat);
	/* TYPEINFO: list<string>(string,boolean,string)*/
	YCPValue SelectionContent (const YCPString&, const YCPBoolean&, const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPBoolean SetSelection (const YCPString&);
	/* TYPEINFO: boolean(string)*/
	YCPValue ClearSelection (const YCPString&);
	/* TYPEINFO: boolean() */
	YCPBoolean ActivateSelections ();

	// package related
	/* TYPEINFO: list<string>(symbol,boolean)*/
	YCPValue GetPackages (const YCPSymbol& which, const YCPBoolean& names_only);
	/* TYPEINFO: list<string>(boolean,boolean,boolean,boolean)*/
	YCPValue FilterPackages (const YCPBoolean& byAuto, const YCPBoolean& byApp, const YCPBoolean& byUser, const YCPBoolean& names_only);
	/* TYPEINFO: boolean(string)*/
	YCPValue IsProvided (const YCPString& tag);
	/* TYPEINFO: boolean(string)*/
	YCPValue IsSelected (const YCPString& tag);
	/* TYPEINFO: boolean(string)*/
	YCPValue IsAvailable (const YCPString& tag);
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
	/* TYPEINFO: boolean()*/
	YCPValue PkgAnyToDelete ();
	/* TYPEINFO: boolean()*/
	YCPValue PkgAnyToInstall ();
	// FIXME: is this needed?
	YCPValue PkgFileHasOwner (YCPList args);
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
	/* TYPEINFO: boolean(boolean)*/
	YCPBoolean PkgSolve (const YCPBoolean& filter);
	/* TYPEINFO: boolean()*/
	YCPBoolean PkgEstablish ();
	/* TYPEINFO: boolean()*/
	YCPBoolean PkgFreshen ();
	/* TYPEINFO: boolean()*/
	YCPBoolean PkgSolveCheckTargetOnly ();
	/* TYPEINFO: integer()*/
	YCPValue PkgSolveErrors ();
	/* TYPEINFO: list<any>(integer)*/
	YCPValue PkgCommit (const YCPInteger& medianr);

	/* FIXME: is this needed? */
	YCPValue PkgPrepareOrder (YCPList args);
	/* TYPEINFO: list<list<integer>>()*/
	YCPValue PkgMediaSizes ();
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

	/* TYPEINFO: boolean(string)*/
	YCPBoolean RpmChecksig( const YCPString & filename );

	// you patch related
	/* TYPEINFO: map<any,any>()*/
        YCPMap YouStatus ();
	/* TYPEINFO: string(list<any>&)*/
	YCPString YouGetServers (YCPReference strings);
	/* TYPEINFO: string(map<any,any>)*/
	YCPValue YouSetServer (const YCPMap& strings);
	/* TYPEINFO: map<any,any>()*/
	YCPValue YouGetUserPassword ();
	/* TYPEINFO: string(string,string,boolean)*/
	YCPValue YouSetUserPassword (const YCPString& user, const YCPString& passwd, const YCPBoolean& persistent);
	/* TYPEINFO: string(boolean,boolean)*/
	YCPValue YouRetrievePatchInfo (const YCPBoolean& download, const YCPBoolean& sign);
	/* TYPEINFO: boolean()*/
	YCPValue YouProcessPatches ();
	/* TYPEINFO: string()*/
	YCPValue YouGetDirectory ();
	/* TYPEINFO: void()*/
	YCPValue YouSelectPatches ();
	/* TYPEINFO: boolean()*/
        YCPValue YouRemovePackages ();
	/* TYPEINFO: boolean(string,symbol)*/
        YCPValue ResolvableInstall( const YCPString& name_r, const YCPSymbol& kind_r );
	/* TYPEINFO: boolean(string,symbol,string,string)*/
	YCPValue ResolvableInstallArchVersion( const YCPString& name_r, const YCPSymbol& kind_r, const YCPString& arch, const YCPString& vers );
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

	// keyring related
	/* TYPEINFO: void(string,boolean)*/
	YCPValue ImportGPGKey(const YCPString& filename, const YCPBoolean& trusted);

	/* TYPEINFO: boolean()*/
	YCPValue SourceReleaseAll ();
	/* TYPEINFO: boolean(string)*/
	YCPValue SourceMoveDownloadArea (const YCPString & path);

        YCPValue ResolvablePropertiesEx(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version, bool dependencies);
	YCPValue ResolvableSetPatches(const YCPSymbol& kind_r, bool preselect);

	/**
	 * Constructor.
	 */
	PkgModuleFunctions ();

	/**
	 * Destructor.
	 */
	virtual ~PkgModuleFunctions ();

	// must be public, used in callbacks
	long long logFindAlias(const std::string &alias) const;

	virtual const string name () const
	{
    	    return "Pkg";
	}

	virtual const string filename () const
	{
    	    return "Pkg";
	}

	virtual string toString () const
	{
    	    return "// not possible toString";
	}

	virtual YCPValue evaluate (bool cse = false )
	{
    	    if (cse) return YCPNull ();
    	    else return YCPVoid ();
	}

	virtual Y2Function* createFunctionCall (const string name, constFunctionTypePtr type);
};
#endif // PkgModuleFunctions_h
