/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                        (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:       PMCB.h

  Author:     Michael Andres <ma@suse.de>
  Maintainer: Michael Andres <ma@suse.de>

  Purpose:

/-*/
#ifndef PMCB_h
#define PMCB_h

#include <iostream>
#include <y2util/Y2SLog.h>

#include <y2pm/Y2PMCallbacks.h>

using namespace std;

#undef Y2LOG
#define Y2LOG "Y2CB"
#define XXX "****CALLBACK: "
#define YYY " @ "

///////////////////////////////////////////////////////////////////
// Reporting progress of rpm database conversion.
///////////////////////////////////////////////////////////////////
struct ConvertDbReceive : public RpmDbCallbacks::ConvertDbCallback {
  ProgressCounter _pc;
  unsigned _lastFailed;
  unsigned _lastIgnored;
  unsigned _lastAlreadyInV4;

  virtual void reportbegin() {
    _pc.reset();
    _lastFailed = _lastAlreadyInV4 = 0;
    SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl;
  }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void start( const Pathname & v3db ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << v3db << YYY << endl;
  }
  virtual void progress( const ProgressData & prg,
			 unsigned failed, unsigned ignored, unsigned alreadyInV4 ) {
    _pc = prg;
    if ( _pc.updateIfNewPercent( 5 )
	 || failed != _lastFailed
	 || ignored != _lastIgnored
	 || alreadyInV4 != _lastAlreadyInV4 ) {
      _lastFailed      = failed;
      _lastIgnored     = ignored;
      _lastAlreadyInV4 = alreadyInV4;
      // report changed values
      MIL << XXX << __PRETTY_FUNCTION__ << YYY << _pc << YYY << failed << YYY << ignored << YYY << alreadyInV4 << YYY << endl;
    }
  }
  virtual void dbInV4( const std::string & pkg ) {
    WAR << XXX << __PRETTY_FUNCTION__ << YYY << pkg << YYY << endl;
    ConvertDbCallback::dbInV4( pkg );
  }
  virtual CBSuggest dbReadError( int offset ) {
    ERR << XXX << __PRETTY_FUNCTION__ << YYY << offset << YYY << endl;
    return CBSuggest::CANCEL;
  }
  virtual void dbWriteError( const std::string & pkg ) {
    WAR << XXX << __PRETTY_FUNCTION__ << YYY << pkg << YYY << endl;
    ConvertDbCallback::dbWriteError( pkg );
  }
  virtual void stop( PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << error << YYY << endl;
  }
  ConvertDbReceive() {
    RpmDbCallbacks::convertDbReport.redirectTo( this );
  }
};

ConvertDbReceive convertDbReceive;

///////////////////////////////////////////////////////////////////
// Reporting progress of rpm database rebuild.
///////////////////////////////////////////////////////////////////
struct RebuildDbCallback : public RpmDbCallbacks::RebuildDbCallback {
  ProgressCounter _pc;
  virtual void reportbegin() {
    _pc.reset();
    SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl;
  }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void start() {
    MIL << XXX << __PRETTY_FUNCTION__ << endl;
  }
  virtual void progress( const ProgressData & prg ) {
    _pc = prg;
    if ( _pc.updateIfNewPercent() ) {
      MIL << XXX << __PRETTY_FUNCTION__ << YYY << _pc << YYY << endl;
    }
  }
  virtual void notify( const string & msg ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << msg << YYY << endl;
  }
  virtual void stop( PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << error << YYY << endl;
  }
  RebuildDbCallback() {
    RpmDbCallbacks::rebuildDbReport.redirectTo( this );
  }
};

RebuildDbCallback rebuildDbCallback;

///////////////////////////////////////////////////////////////////
// Reporting progress reading the rpm database
///////////////////////////////////////////////////////////////////
struct ScanDbCallback : public RpmDbCallbacks::ScanDbCallback {
  ProgressCounter _pc;
  virtual void reportbegin() {
    _pc.reset();
    SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl;
  }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void start() {
    MIL << XXX << __PRETTY_FUNCTION__ << endl;
  }
  virtual void progress( const ProgressData & prg ) {
    _pc = prg;
    if ( 1||_pc.updateIfNewPercent() ) {
      MIL << XXX << __PRETTY_FUNCTION__ << YYY << _pc << YYY << endl;
    }
  }
  virtual void stop( PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << error << YYY << endl;
  }
  ScanDbCallback() {
    RpmDbCallbacks::scanDbReport.redirectTo( this );
  }
};

ScanDbCallback scanDbCallback;

///////////////////////////////////////////////////////////////////
// Reporting progress of rpm package installation.
///////////////////////////////////////////////////////////////////
struct InstallPkgCallback : public RpmDbCallbacks::InstallPkgCallback {
  virtual void reportbegin() { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void start( const Pathname & filename ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << filename << YYY << endl;
  }
  virtual void progress( const ProgressData & prg ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << prg << YYY << endl;
  }
  virtual void stop( PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << error << YYY << endl;
  }
  InstallPkgCallback() {
    RpmDbCallbacks::installPkgReport.redirectTo( this );
  }
};

InstallPkgCallback installPkgCallback;

///////////////////////////////////////////////////////////////////
// Reporting progress of rpm package deletion.
///////////////////////////////////////////////////////////////////
struct RemovePkgCallback : public RpmDbCallbacks::RemovePkgCallback {
  virtual void reportbegin() { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void start( const std::string & label ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << label << YYY << endl;
  }
  virtual void progress( const ProgressData & prg ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << prg << YYY << endl;
  }
  virtual void stop( PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << error << YYY << endl;
  }
  RemovePkgCallback() {
    RpmDbCallbacks::removePkgReport.redirectTo( this );
  }
};

RemovePkgCallback removePkgCallback;

///////////////////////////////////////////////////////////////////
// Reporting progress of download
///////////////////////////////////////////////////////////////////
struct DownloadProgressCallback : public MediaCallbacks::DownloadProgressCallback {
  virtual void reportbegin() { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void start( const Url & url_r, const Pathname & localpath_r ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << url_r << YYY << localpath_r << YYY << endl;
  }
  virtual void progress( const ProgressData & prg ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << prg << YYY << endl;
  }
  virtual void stop( PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << error << YYY << endl;
  }
  DownloadProgressCallback() {
    MediaCallbacks::downloadProgressReport.redirectTo( this );
  }
};

DownloadProgressCallback downloadProgressCallback;

///////////////////////////////////////////////////////////////////
// Reporting progress of download
///////////////////////////////////////////////////////////////////
#undef Y2LOG
#define Y2LOG "MediaChangeCallback"
struct MediaChangeCallback  : public InstSrcManagerCallbacks::MediaChangeCallback  {
  virtual void reportbegin() { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }
  virtual void reportend()   { SEC << XXX << __PRETTY_FUNCTION__ << YYY << endl; }

  virtual bool isSet() {
    MIL << XXX << __PRETTY_FUNCTION__ << YYY << endl;
    return true;
  }

  virtual std::string changeMedia( constInstSrcDescrPtr descr,
				   const Url & currentUrl,
				   int expectedMedianr,
				   PMError error ) {
    MIL << XXX << __PRETTY_FUNCTION__ << endl
      << "  " << YYY << descr << endl
      << "  " << YYY << currentUrl << endl
      << "  " << YYY << expectedMedianr << endl
      << "  " << YYY << error << endl
      << "  " << endl;
    static string nret( "x" );
    string ret( nret );
    nret = "S";
    return ret;
  }

  virtual std::string changeMedia( const std::string & error,
				   const std::string & url,
				   const std::string & product,
				   int current,
				   const std::string & currentLabel,
				   int expected,
				   const std::string & expectedLabel,
				   bool doublesided ) {
    return "S";
  }
  MediaChangeCallback() {
    InstSrcManagerCallbacks::mediaChangeReport.redirectTo( this );
  }
};

MediaChangeCallback mediaChangeCallback;

#undef Y2LOG
#define Y2LOG "DEFINE_Y2LOG"
#endif // PMCB_h
