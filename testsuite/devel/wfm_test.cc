#include <iomanip>
#include <fstream>
#include <string>
#include <list>

#include <y2util/Y2SLog.h>
#include <y2util/Date.h>
#include <y2util/stringutil.h>
#include <y2util/ExternalProgram.h>

#include <Y2PM.h>
#include <y2pm/RpmDb.h>
#include <y2pm/librpmDb.h>

#include <y2pm/InstSrcManager.h>
#include <y2pm/InstSrc.h>
#include <y2pm/InstSrcDescr.h>
#include <y2pm/MediaAccess.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>
#include <y2pm/InstTarget.h>
#include <y2pm/Timecount.h>
#include <y2pm/PMPackageImEx.h>

#include <YCP.h>
#include <ycp/y2log.h>
#include <PkgModuleFunctions.h>

#include "PMCB.h"

using namespace std;

#define TMGR Y2PM::instTarget()
#define PMGR Y2PM::packageManager()
#define SMGR Y2PM::selectionManager()
#define ISM  Y2PM::instSrcManager()

extern ostream & operator<<( ostream & str, const YCPValue & val );

ostream & operator <<( ostream & str, const list<string> & t ) {
  stringutil::dumpOn( str, t, true );
  return str;
}

ostream & operator<<( ostream & str, const PkgSplitSet & obj ) {
  for ( PkgSplitSet::const_iterator it = obj.begin(); it != obj.end(); ++it ) {
    str << *it << endl;
  }
  return str;
}

void dataDump( ostream & str, constPMPackagePtr p ) {
  str << p << " ++++++++++++++++++++++++++++" << endl;
  str << "SUMMARY:       " << p->summary() << endl;
  str << "DESCRIPTION:   " << p->description() << endl;
  str << "INSNOTIFY:     " << p->insnotify() << endl;
  str << "DELNOTIFY:     " << p->delnotify() << endl;
  str << "SIZE:          " << p->size() << endl;
  str << "INSTSRCLABEL:  " << p->instSrcLabel() << endl;
  str << "INSTSRCVENDOR: " << p->instSrcVendor() << endl;
  str << "INSTSRCRANK:   " << p->instSrcRank() << endl;
  str << "BUILDTIME:     " << p->buildtime() << endl;
  str << "BUILDHOST:     " << p->buildhost() << endl;
  str << "INSTALLTIME:   " << p->installtime() << endl;
  str << "DISTRIBUTION:  " << p->distribution() << endl;
  str << "VENDOR:        " << p->vendor() << endl;
  str << "LICENSE:       " << p->license() << endl;
  str << "PACKAGER:      " << p->packager() << endl;
  str << "GROUP:         " << p->group() << endl;
  str << "CHANGELOG:     " << p->changelog() << endl;
  str << "URL:           " << p->url() << endl;
  str << "OS:            " << p->os() << endl;
  str << "PREIN:         " << p->prein() << endl;
  str << "POSTIN:        " << p->postin() << endl;
  str << "PREUN:         " << p->preun() << endl;
  str << "POSTUN:        " << p->postun() << endl;
  str << "SOURCELOC:     " << p->sourceloc() << endl;
  str << "SOURCESIZE:    " << p->sourcesize() << endl;
  str << "ARCHIVESIZE:   " << p->archivesize() << endl;
  str << "AUTHORS:       " << p->authors() << endl;
  str << "FILENAMES:     " << p->filenames() << endl;
  str << "RECOMMENDS:    " << p->recommends() << endl;
  str << "SUGGESTS:      " << p->suggests() << endl;
  str << "LOCATION:      " << p->location() << endl;
  str << "MEDIANR:       " << p->medianr() << endl;
  str << "KEYWORDS:      " << p->keywords() << endl;
  str << "INSTSOURCE:    " << p->source() << endl;
  str << "IS REMOTE:     " << p->isRemote() << endl;
  str << "PROVIDES:      " << p->provides().size() << endl;
  str << "REQUIRES:      " << p->requires().size() << endl;
  str << "CONFLICTS:     " << p->conflicts().size() << endl;
  str << "OBSOLETES:     " << p->obsoletes().size() << endl;
  str << "PREREQUIRES:   " << p->prerequires().size() << endl;
  str << "SPLITPROVIDES: " << p->splitprovides().size() << endl;
  str << "---------------" << endl;
}

void dummyDU()
{
  std::set<PkgDuMaster::MountPoint> mountpoints;
  mountpoints.insert( PkgDuMaster::MountPoint( "/",          FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/boot",      FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/bin",       FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/etc",       FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/lib",       FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/opt",       FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/sbin",      FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/usr",       FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/usr/local", FSize(4,FSize::K), FSize(1,FSize::G) ) );
  mountpoints.insert( PkgDuMaster::MountPoint( "/var",       FSize(4,FSize::K), FSize(1,FSize::G) ) );
  MIL << mountpoints;
  PMGR.setMountPoints( mountpoints );
}

InstSrcManager::ISrcId newSrc( const string & url_r ) {
  Url url( url_r );
  InstSrcManager::ISrcIdList idlist;
  Timecount _t( url_r.c_str() );
  PMError err = ISM.scanMedia( idlist, url_r );
  ( err ? ERR : MIL ) << "newSrc: " << idlist.size() << " (" << err << ")" << endl;
  return( idlist.size() ? *idlist.begin() : 0 );
}

ostream & dumpPkgWhatIf( ostream & str, bool all = false )
{
  str << "+++[dumpPkgWhatIf]+++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  for ( PMManager::PMSelectableVec::const_iterator it = PMGR.begin(); it != PMGR.end(); ++it ) {
    if ( all || (*it)->to_modify() || (*it)->is_taboo() ) {
      (*it)->dumpStateOn( str ) << endl;
    }
  }
  str << "---[dumpPkgWhatIf]---------------------------------------------------" << endl;
  return str;
}

ostream & dumpSelWhatIf( ostream & str, bool all = false  )
{
  str << "+++[dumpSelWhatIf]+++++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
  for ( PMManager::PMSelectableVec::const_iterator it = SMGR.begin(); it != SMGR.end(); ++it ) {
    if ( all || (*it)->to_modify() ) {
      (*it)->dumpStateOn( str ) << endl;
    }
  }
  str << "---[dumpSelWhatIf]---------------------------------------------------" << endl;
  return str;
}

/******************************************************************
 ******************************************************************/

struct WFM {
  PkgModuleFunctions * _pkgmod;
#define OUT SEC

  WFM()
  {
    _pkgmod = 0;
  }
  ~WFM() {
    //delete _pkgmod;
  }
  void init() {
    if ( !_pkgmod ) {
      _pkgmod = new PkgModuleFunctions();
    }
  }
  void close() {
    delete _pkgmod;
    _pkgmod = 0;
  }

  void SourceStartManager( bool ena ) {
    YCPList args;
    args->add( YCPBoolean(ena) );
    OUT << "SourceStartManager" << args;
    YCPValue ret = _pkgmod->SourceStartManager( YCPBoolean(ena) );
    OUT << " --> " << ret << endl;
  }
  void SourceStartCache( bool ena ) {
    YCPList args;
    args->add( YCPBoolean(ena) );
    OUT << "SourceStartCache" << args;
    YCPValue ret = _pkgmod->SourceStartCache( YCPBoolean(ena) );
    OUT << " --> " << ret << endl;
  }
  void SourceGetCurrent( bool ena ) {
    YCPList args;
    args->add( YCPBoolean(ena) );
    OUT << "SourceGetCurrent" << args;
    YCPValue ret = _pkgmod->SourceGetCurrent( YCPBoolean(ena) );
    OUT << " --> " << ret << endl;
  }
  void SourceProduct( int id ) {
    YCPList args;
    args->add( YCPInteger(id) );
    OUT << "SourceProduct" << args;
    YCPValue ret = _pkgmod->SourceProduct( YCPInteger(id) );
    OUT << " --> " << ret << endl;
  }
  void SourceGeneralData( int id ) {
    YCPList args;
    args->add( YCPInteger(id) );
    OUT << "SourceGeneralData" << args;
    YCPValue ret = _pkgmod->SourceGeneralData( YCPInteger(id) );
    OUT << " --> " << ret << endl;
  }
  void SourceCreate( const string & url_r ) {
    YCPList args;
    args->add( YCPString(url_r) );
    OUT << "SourceCreate" << args;
    YCPValue ret = _pkgmod->SourceCreate( YCPString(url_r), YCPString("/") );
    OUT << " --> " << ret << endl;
  }
  void SourceSetEnabled( int id, bool ena ) {
    YCPList args;
    args->add( YCPInteger(id) );
    args->add( YCPBoolean(ena) );
    OUT << "SourceSetEnabled" << args;
    YCPValue ret = _pkgmod->SourceSetEnabled( YCPInteger(id), YCPBoolean(ena) );
    OUT << " --> " << ret << endl;
  }
  void SourceEditGet() {
    YCPList args;
    OUT << "SourceEditGet" << args;
    YCPValue ret = _pkgmod->SourceEditGet();
    OUT << " --> " << ret << endl;
  }
  void SourceEditSet( const InstSrcManager::SrcStateVector & source_states ) {
    YCPList args;

    YCPList a1;
    for ( InstSrcManager::SrcStateVector::const_iterator it = source_states.begin();
	  it != source_states.end(); ++it ) {
      YCPMap el;
      el->add( YCPString("SrcId"),	YCPInteger( it->first ) );
      el->add( YCPString("enabled"),	YCPBoolean( it->second ) );
      a1->add( el );
    }
    args->add( a1 );

    OUT << "SourceEditSet" << args;
    YCPValue ret = _pkgmod->SourceEditSet( a1 );
    OUT << " --> " << ret << endl;
  }
  void PkgGetLicenseToConfirm( const string & name_r ) {
    YCPList args;
    args->add( YCPString(name_r) );
    OUT << "PkgGetLicenseToConfirm" << args;
    YCPValue ret = _pkgmod->PkgGetLicenseToConfirm(YCPString(name_r));
    OUT << " --> " << ret << endl;
  }
  void PkgGetLicensesToConfirm( const list<string> & name_r ) {
    YCPList args;

    YCPList a1;
    for ( list<string>::const_iterator it = name_r.begin(); it != name_r.end(); ++it ) {
      a1->add( YCPString( *it ) );
    }
    args->add( a1 );

    OUT << "PkgGetLicensesToConfirm" << args;
    YCPValue ret = _pkgmod->PkgGetLicensesToConfirm(a1);
    OUT << " --> " << ret << endl;
  }
  void PkgGetFilelist( const string & name_r ) {
    YCPList args;
    args->add( YCPString(name_r) );
    OUT << "PkgGetFilelist" << args;
    YCPValue ret = _pkgmod->PkgGetFilelist(YCPString(name_r),YCPSymbol("installed"));
    OUT << "i --> " << ret << endl;
    ret = _pkgmod->PkgGetFilelist(YCPString(name_r),YCPSymbol("candidate"));
    OUT << "c --> " << ret << endl;
    ret = _pkgmod->PkgGetFilelist(YCPString(name_r),YCPSymbol("XXX"));
    OUT << "X --> " << ret << endl;
  }
  void PkgSolveCheckTargetOnly() {
    YCPList args;
    OUT << "PkgSolveCheckTargetOnly" << args;
    YCPValue ret = _pkgmod->PkgSolveCheckTargetOnly();
    OUT << " --> " << ret << endl;
  }

  void PkgMediaNames() {
    YCPList args;
    OUT << "PkgMediaNames" << args;
    YCPValue ret = _pkgmod->PkgMediaNames();
    OUT << " --> " << ret << endl;
  }
  void PkgMediaSizes() {
    YCPList args;
    OUT << "PkgMediaSizes" << args;
    YCPValue ret = _pkgmod->PkgMediaSizes();
    OUT << " --> " << ret << endl;
  }
  void PkgMediaCount() {
    YCPList args;
    OUT << "PkgMediaCount" << args;
    YCPValue ret = _pkgmod->PkgMediaCount();
    OUT << " --> " << ret << endl;
  }
  void PkgCommit() {
    YCPList args;
    YCPInteger i( (long long int)0 );
    args->add( i );
    OUT << "PkgCommit" << args;
    YCPValue ret = _pkgmod->PkgCommit( i );
    OUT << " --> " << ret << endl;
  }

};

static WFM wfm;

void WFMtest() {
  wfm.init();
  INT << "WFM START" << endl;
  wfm.SourceStartManager( false );
  wfm.SourceStartCache( true );
  Y2PM::instTargetInit("/");



  for ( PMManager::PMSelectableVec::const_iterator it = PMGR.begin(); it != PMGR.end(); ++it ) {
      (*it)->auto_set_install();
      (*it)->set_source_install( true );
  }

  //dumpPkgWhatIf( SEC );

  wfm.PkgMediaNames();
  wfm.PkgMediaSizes();
  wfm.PkgMediaCount();

  wfm.PkgCommit();

  INT << "WFM STOP" << endl;
  wfm.close();
}

/******************************************************************
**
**
**	FUNCTION NAME : main
**	FUNCTION TYPE : int
**
**	DESCRIPTION :
*/
int main()
{
  y2error( "xxx" );
  set_log_filename( "-" );
  MIL << "START" << endl;

  if ( 0 ) {
    //Y2PM::noAutoInstSrcManager();
    Timecount _t("",false);
    _t.start( "Launch InstTarget" );
    Y2PM::instTargetInit("/");
    _t.start( "Launch PMPackageManager" );
    Y2PM::packageManager();
    _t.start( "Launch PMSelectionManager" );
    Y2PM::selectionManager();
    _t.start( "Launch InstSrcManager" );
    Y2PM::instSrcManager();
    _t.stop();
    INT << "Total Packages "   << PMGR.size() << endl;
    INT << "Total Selections " << SMGR.size() << endl;
  }

  //InstSrcManager::ISrcId nid1 = newSrc( "/schnell/CD-ARCHIVE/SLES9/SLES-9-i386-RC5/sles9-i386/CD1" );
  //InstSrcManager::ISrcId nid2 = newSrc( "/schnell/CD-ARCHIVE/SLES9/SLES-9-i386-RC5/core9-i386/CD1" );

  // wfm.init();
  WFMtest();
  // wfm.close();



  SEC << "STOP" << endl;
  return 0;
}


