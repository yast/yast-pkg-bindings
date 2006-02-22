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
#include <zypp/Pathname.h>
#include <zypp/Url.h>


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

	/** 
	 * ZYPP
	 */
	zypp::ZYpp::Ptr zypp_ptr;


    private: // source related

      bool DoProvideNameKind( const std::string & name , zypp::Resolvable::Kind kind);
      bool DoRemoveNameKind( const std::string & name, zypp::Resolvable::Kind kind);
      bool DoProvideAllKind(zypp::Resolvable::Kind kind);
      bool DoRemoveAllKind(zypp::Resolvable::Kind kind);
      bool DoAllKind(zypp::Resolvable::Kind kind, bool provide);
      YCPValue GetPkgLocation(const YCPString& p, bool full_path);
      
      // builtin handling
      void registerFunctions ();
      vector<string> _registered_functions;

    private:

      /**
       * Handler for YCPCallbacks received or triggered.
       * Needs access to WFM. See @ref CallbackHandler.
       **/
      CallbackHandler & _callbackHandler;


    public:
	// general
	/* TYPEINFO: void() */
	YCPValue InstSysMode ();
	/* TYPEINFO: void(string) */
	YCPValue SetLocale (const YCPString& locale);
	/* TYPEINFO: string() */
	YCPValue GetLocale ();
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
	YCPValue CallbackStartDownload (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackProgressDownload (const YCPString& func);
	/* TYPEINFO: void(string) */
	YCPValue CallbackDoneDownload (const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackStartSourceRefresh( const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackErrorSourceRefresh( const YCPString& func);
	/* TYPEINFO: void(string) */
        YCPValue CallbackDoneSourceRefresh( const YCPString& func);
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

	// source related
	/* TYPEINFO: boolean(boolean)*/
        YCPValue SourceStartManager (const YCPBoolean&);
	/* TYPEINFO: integer(string,string)*/
	YCPValue SourceCreate (const YCPString&, const YCPString&);
	/* TYPEINFO: list<integer>(boolean)*/
	YCPValue SourceStartCache (const YCPBoolean&);
	/* TYPEINFO: list<integer>(boolean)*/
	YCPValue SourceGetCurrent (const YCPBoolean& enabled);
	/* TYPEINFO: boolean(integer)*/
	YCPValue SourceFinish (const YCPInteger&);
	/* TYPEINFO: boolean()*/
	YCPValue SourceFinishAll ();
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceGeneralData (const YCPInteger&);
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceMediaData (const YCPInteger&);
	/* TYPEINFO: map<string,any>(integer)*/
	YCPValue SourceProductData (const YCPInteger&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideFile (const YCPInteger&, const YCPInteger&, const YCPString&);
	/* TYPEINFO: string(integer,integer,string)*/
	YCPValue SourceProvideDir (const YCPInteger&, const YCPInteger&, const YCPString&);
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

	// target related
	/* TYPEINFO: boolean(string,boolean)*/
	YCPValue TargetInit (const YCPString& root, const YCPBoolean& n);
	/* TYPEINFO: boolean()*/
	YCPBoolean TargetFinish ();
	/* TYPEINFO: boolean(string)*/
	YCPBoolean TargetLogfile (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetCapacity (const YCPString&);
	/* TYPEINFO: integer(string)*/
	YCPInteger TargetUsed (const YCPString&);
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

	// selection related
	/* TYPEINFO: list<string>(symbol,string)*/
	YCPValue GetSelections (const YCPSymbol& stat, const YCPString& cat);
	/* TYPEINFO: string()*/
	YCPValue GetBackupPath ();
	/* TYPEINFO: void(string)*/
	YCPValue SetBackupPath (const YCPString& path);
	/* TYPEINFO: void(boolean)*/
	YCPValue CreateBackups (const YCPBoolean& flag);
	/* TYPEINFO: map<string,any>(string)*/
	YCPValue SelectionData (const YCPString& cat);
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
	/* TYPEINFO: list<string>(string,symbol)*/
	YCPList  PkgGetFilelist (const YCPString& package, const YCPSymbol& which);
	/* TYPEINFO: boolean()*/
	YCPValue IsManualSelection ();
	/* TYPEINFO: boolean()*/
	YCPValue ClearSaveState ();
	/* TYPEINFO: boolean()*/
	YCPValue SaveState ();
	/* TYPEINFO: boolean(boolean)*/
	YCPValue RestoreState (const YCPBoolean&);
	/* TYPEINFO: map<symbol,integer>(boolean)*/
	YCPMap   PkgUpdateAll (const YCPBoolean& del);
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
	/* TYPEINFO: boolean(boolean)*/
	YCPBoolean PkgSolve (const YCPBoolean& filter);
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
	/* TYPEINFO: list<string>()*/
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
	/* TYPEINFO: boolean(string,symbol)*/
        YCPValue ResolvableRemove( const YCPString& name_r, const YCPSymbol& kind_r );
	/* TYPEINFO: list<map<string,any> >(string,symbol,string)*/
        YCPValue ResolvableProperties(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version);

	/**
	 * Constructor.
	 */
	PkgModuleFunctions ();

	/**
	 * Destructor.
	 */
	virtual ~PkgModuleFunctions ();

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

    protected:

	struct MountPoint {
	    std::string dir;
	    mutable long long total_size;
	    mutable long long used_size;
	    mutable long long pkg_size;
	    bool readonly;

	    MountPoint() : dir(""), total_size(0LL), used_size(0LL), pkg_size(0LL), readonly(false) {}

	    MountPoint(std::string d, long long total, long long used, long long pkg, bool ro) :
	    dir(d), total_size(total), used_size(used), pkg_size(pkg), readonly(ro) {} 

	    // order by directory name
	    bool operator<( const MountPoint & rhs ) const {
		return dir < rhs.dir;
	    }
	};

	// the set is sorted by directory name
	std::set<MountPoint> _mount_points;

};
#endif // PkgModuleFunctions_h
