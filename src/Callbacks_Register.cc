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
   Summary:     Functions for registering YCP callbacks from Yast
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "Callbacks.h"
#include "Callbacks.YCP.h" // PkgFunctions::CallbackHandler::YCPCallbacks
#include "log.h"


///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgFunctions
//
//      Set YCPCallbacks.  _ycpCallbacks
//
///////////////////////////////////////////////////////////////////

#define SET_YCP_CB(E,A) _callbackHandler._ycpCallbacks.setYCPCallback( CallbackHandler::YCPCallbacks::E, A );

YCPValue PkgFunctions::CallbackStartProvide( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartProvide, args );
}
YCPValue PkgFunctions::CallbackProgressProvide( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressProvide, args );
}
YCPValue PkgFunctions::CallbackDoneProvide( const YCPValue& args ) {
  return SET_YCP_CB( CB_DoneProvide, args );
}

YCPValue PkgFunctions::CallbackStartPackage( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartPackage, args );
}
YCPValue PkgFunctions::CallbackProgressPackage( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressPackage, args );
}
YCPValue PkgFunctions::CallbackDonePackage( const YCPValue& args ) {
  return SET_YCP_CB( CB_DonePackage, args );
}

YCPValue PkgFunctions::CallbackResolvableReport( const YCPValue& args ) {
  return SET_YCP_CB( CB_ResolvableReport, args );
}

/**
 * @builtin CallbackImportGpgKey
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(map<string,any> key, integer repo_id)</code>. The callback function should ask user whether the key is trusted and can be imported, returned 'true' value means to import the trusted key
 * @return void
 */
YCPValue PkgFunctions::CallbackImportGpgKey( const YCPValue& args ) {
  return SET_YCP_CB( CB_ImportGpgKey, args );
}

/**
 * @builtin CallbackAcceptUnknownGpgKey
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string keyid)</code>. The callback function should ask user whether the unknown key can be accepted, returned true value means to accept the key.
 * @return void
 */
YCPValue PkgFunctions::CallbackAcceptUnknownGpgKey( const YCPValue& args ) {
  return SET_YCP_CB( CB_AcceptUnknownGpgKey, args );
}

/**
 * @builtin CallbackAcceptUnsignedFile
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename)</code>. The callback function should ask user whether the unsigned file can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgFunctions::CallbackAcceptUnsignedFile( const YCPValue& args ) {
  return SET_YCP_CB( CB_AcceptUnsignedFile, args );
}

/**
 * @builtin CallbackAcceptFileWithoutChecksum
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename)</code>. The callback function should ask user whether the unsigned file can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgFunctions::CallbackAcceptFileWithoutChecksum( const YCPValue& args ) {
  return SET_YCP_CB( CB_AcceptFileWithoutChecksum, args );
}

/**
 * @builtin CallbackAcceptVerificationFailed
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, map<string,any> key)</code>. The callback function should ask user whether the unsigned file can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgFunctions::CallbackAcceptVerificationFailed( const YCPValue& args ) {
  return SET_YCP_CB( CB_AcceptVerificationFailed, args );
}

/**
 * @builtin CallbackAcceptWrongDigest
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string requested_digest, string found_digest)</code>. The callback function should ask user whether the wrong digest can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgFunctions::CallbackAcceptWrongDigest( const YCPValue& args)
{
  return SET_YCP_CB( CB_AcceptWrongDigest, args);
}

/**
 * @builtin CallbackAcceptUnknownDigest
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string name)</code>. The callback function should ask user whether the uknown digest can be accepted, returned true value means to accept the digest.
 * @return void
 */
YCPValue PkgFunctions::CallbackAcceptUnknownDigest( const YCPValue& args)
{
  return SET_YCP_CB( CB_AcceptUnknownDigest, args);
}

/**
 * @builtin CallbackTrustedKeyAdded
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string keyid, string keyname)</code>. The callback function should inform user that a trusted key has been added.
 * @return void
 */
YCPValue PkgFunctions::CallbackTrustedKeyAdded( const YCPValue& args ) {
  return SET_YCP_CB( CB_TrustedKeyAdded, args );
}

/**
 * @builtin CallbackTrustedKeyRemoved
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string keyid, string keyname)</code>. The callback function should inform user that a trusted key has been removed.
 * @return void
 */
YCPValue PkgFunctions::CallbackTrustedKeyRemoved( const YCPValue& args ) {
  return SET_YCP_CB( CB_TrustedKeyRemoved, args );
}

/**
 * @builtin CallbackMediaChange
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>string( string error_code, string error, string url, string product, integer current, string current_label, integer wanted, string wanted_label, boolean double_sided, list<string> devices, integer current_device )</code>. The callback function should ask user to change the medium or to change the URL of the repository. Error code can be "NO_ERROR" (no error occured), "NOT_FOUND" (the file was not found), "IO" (I/O error), "IO_SOFT" (timeout when acceessing the network), "INVALID" (broken medium), "WRONG" (unexpected medium, wrong ID). Result of the callback must be "" (retry), "I" (ignore), "C" (cancel), "S" (skip), "E" (eject), "E+number" (eject device at number) or a new URL of the repository.
 * @return void
 */
YCPValue PkgFunctions::CallbackMediaChange( const YCPValue& args ) {
  // FIXME: Allow omission of 'src' argument in 'src, name'. Since we can
  // handle one callback function at most, passing a src argument
  // implies a per-source callback which isn't implemented anyway.
  return SET_YCP_CB( CB_MediaChange, args );
}

YCPValue PkgFunctions::CallbackSourceChange( const YCPValue& args ) {
  return SET_YCP_CB( CB_SourceChange, args );
}

YCPValue PkgFunctions::CallbackStartRebuildDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartRebuildDb, args );
}
YCPValue PkgFunctions::CallbackProgressRebuildDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressRebuildDb, args );
}
YCPValue PkgFunctions::CallbackNotifyRebuildDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_NotifyRebuildDb, args );
}
YCPValue PkgFunctions::CallbackStopRebuildDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_StopRebuildDb, args );
}



/**
 * @builtin CallbackStartScanDb
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the RPM DB reading has been started.
 * @return void
 */
YCPValue PkgFunctions::CallbackStartScanDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartScanDb, args );
}
/**
 * @builtin CallbackProgressScanDb
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(integer percent)</code>. The callback function is evaluated during RPM DB reading.
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressScanDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressScanDb, args );
}
/**
 * @builtin CallbackErrorScanDb
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer error_code, string description)</code>. The callback function is evaluated when an error occurrs during RPM DB reading. error_code 0 means no error.
 * @return void
 */
YCPValue PkgFunctions::CallbackErrorScanDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_ErrorScanDb, args );
}
/**
 * @builtin CallbackDoneScanDb
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer error_code, string description)</code>. The callback function is evaluated when RPM DB reading is finished. error_code 0 means no error.
 * @return void
 */
YCPValue PkgFunctions::CallbackDoneScanDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_DoneScanDb, args );
}

/** Legacy: Unused since Code12 */
YCPValue PkgFunctions::CallbackStartConvertDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartConvertDb, args );
}
/** Legacy: Unused since Code12 */
YCPValue PkgFunctions::CallbackProgressConvertDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressConvertDb, args );
}
/** Legacy: Unused since Code12 */
YCPValue PkgFunctions::CallbackNotifyConvertDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_NotifyConvertDb, args );
}
/** Legacy: Unused since Code12 */
YCPValue PkgFunctions::CallbackStopConvertDb( const YCPValue& args ) {
  return SET_YCP_CB( CB_StopConvertDb, args );
}


/**
 * @builtin CallbackStartDeltaDownload
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string filename, integer download_size)</code>. If the download size is unknown download_size is 0. The callback function is evaluated when a delta RPM download has been started.
 * @return void
 */
YCPValue PkgFunctions::CallbackStartDeltaDownload( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartDeltaDownload, args);
}

/**
 * @builtin CallbackProgressDeltaDownload
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean (integer value)</code>. The callback function is evaluated when more than 5% of the size has been downloaded since the last evaluation. If the handler returns false the download is aborted.
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressDeltaDownload( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressDeltaDownload, args);
}

/**
 * @builtin CallbackProblemDeltaDownload
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string description)</code>. The callback function should inform user that a problem has occurred during delta file download. This is not fatal, it still may be possible to download the full RPM instead.
 * @return void
 */
YCPValue PkgFunctions::CallbackProblemDeltaDownload( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProblemDeltaDownload, args);
}

/**
 * @builtin CallbackStartDeltaApply
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string filename)</code>. The callback function should inform user that a delta application has been started.
 * @return void
 */
YCPValue PkgFunctions::CallbackStartDeltaApply( const YCPValue& args ) {
  return SET_YCP_CB( CB_StartDeltaApply, args);
}

/**
 * @builtin CallbackProgressDeltaApply
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer value)</code>. The callback function is evaluated when more than 5% of the delta size has been applied since the last evaluation.
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressDeltaApply( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProgressDeltaApply, args);
}

/**
 * @builtin CallbackProblemDeltaApply
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string description)</code>. The callback function should inform user that a problem has occurred during delta file application. This is not fatal, it still may be possible to use the full RPM instead.
 * @return void
 */
YCPValue PkgFunctions::CallbackProblemDeltaApply( const YCPValue& args ) {
  return SET_YCP_CB( CB_ProblemDeltaApply, args);
}

/**
 * @builtin CallbackFinishDeltaDownload
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the delta download has been finished.
 * @return void
 */
YCPValue PkgFunctions::CallbackFinishDeltaDownload( const YCPValue& args)
{
    return SET_YCP_CB( CB_FinishDeltaDownload, args);
}

/**
 * @builtin CallbackFinishDeltaApply
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the delta download has been applied.
 * @return void
 */
YCPValue PkgFunctions::CallbackFinishDeltaApply( const YCPValue& args)
{
    return SET_YCP_CB( CB_FinishDeltaApply, args);
}

/**
 * @builtin CallbackPkgGpgCheck
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when a package GPG signature is checked.
 * @return void
 */
YCPValue PkgFunctions::CallbackPkgGpgCheck( const YCPValue& args)
{
    return SET_YCP_CB( CB_PkgGpgCheck, args);
}

/**
 * @builtin CallbackSourceCreateStart
 * @short Register callback function
 * @deprecated libzypp never emit it
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string url)</code>. The callback is evaluated when a source creation has been started.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceCreateStart( const YCPValue& args)
{
    return YCPVoid();
}


/**
 * @builtin CallbackSourceProgressData
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. The callback function is evaluated when more than 5% of the data has been processed since the last evaluation. If the handler returns false the download is aborted.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceCreateProgress( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceCreateError
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>string(string url, string err_code, string description)</code>. err_code is "NO_ERROR", "NOT_FOUND" (the URL was not found), "IO" (I/O error) or "INVALID" (the source is not valid). The callback function must return "ABORT" or "RETRY". The callback function is evaluated when an error occurrs during creation of the source.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceCreateError( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceCreateEnd
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string url, string err_code, string description)</code>. err_code is "NO_ERROR", "NOT_FOUND" (the URL was not found), "IO" (I/O error) or "INVALID" (the source is not valid). The callback function is evaluated when creation of the source has been finished.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceCreateEnd( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceProbeStart
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string url)</code>. The callback function is evaluated when source probing has been started.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceProbeStart( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceProbeFailed
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string url, string type)</code>. The callback function is evaluated when the probed source has different type.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceProbeFailed( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceProbeSucceeded
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string url, string type)</code>. The callback function is evaluated when the probed source has type <code>type</code>.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceProbeSucceeded( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceProbeEnd
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string url, string error, string reason)</code>. The callback function is evaluated when source probing has been finished.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceProbeEnd( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceProbeProgress
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. If the handler returns false the refresh is aborted.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceProbeProgress( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceProbeError
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>string(string url, string error, string reason)</code>. The callback function is evaluated when an error occurrs. The callback function must return string "ABORT" or "RETRY".
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceProbeError( const YCPValue& args)
{
    return YCPVoid();
}

YCPValue PkgFunctions::CallbackSourceReportInit( const YCPValue& args)
{
    return YCPVoid();
}

YCPValue PkgFunctions::CallbackSourceReportDestroy( const YCPValue& args)
{
    return YCPVoid();
}

YCPValue PkgFunctions::CallbackSourceCreateInit( const YCPValue& args)
{
    return YCPVoid();
}

YCPValue PkgFunctions::CallbackSourceCreateDestroy( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceReportStart
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer source_id, string url, string task)</code>.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceReportStart( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceReportProgress
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. If the handler returns false the task is aborted.
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceReportProgress( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceReportError
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>string(integer numeric_id, string url, string error, string reason)</code>. Parameter error is "NO_ERROR", "NOT_FOUND", "IO" or "INVALID". The callback function is evaluated when an error occurrs. The callback function must return string "ABORT", "IGNORE" or "RETRY".
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceReportError( const YCPValue& args)
{
    return YCPVoid();
}

/**
 * @builtin CallbackSourceReportEnd
 * @deprecated libzypp never emit it
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer numeric_id, string url, string task, string error, string reason)</code>. Parameter error is "NO_ERROR", "NOT_FOUND", "IO" or "INVALID". The callback function is evaluated when an error occurrs. The callback function must return string "ABORT", "IGNORE" or "RETRY".
 * @return void
 */
YCPValue PkgFunctions::CallbackSourceReportEnd( const YCPValue& args)
{
    return YCPVoid();
}

YCPValue PkgFunctions::CallbackStartDownload( const YCPValue& args ) {
    return SET_YCP_CB( CB_StartDownload, args );
}
/**
 * @builtin CallbackProgressDownload
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(integer percent, integer average_bps, integer current_bps)</code>. The callback function is evaluated when at least 5% has been downloaded. If the callback function returns false the download is aborted.
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressDownload( const YCPValue& args ) {
    return SET_YCP_CB( CB_ProgressDownload, args );
}
YCPValue PkgFunctions::CallbackDoneDownload( const YCPValue& args ) {
    return SET_YCP_CB( CB_DoneDownload, args );
}


/**
 * @builtin CallbackScriptStart
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string patch_name, string patch_version, string patch_arch, string script_path, boolean installation)</code>. Parameter 'installation' is true when the script is called during installation of a patch, false means patch removal. The callback function is evaluated when a script (which is part of a patch) has been started.
 * @return void
 */
YCPValue PkgFunctions::CallbackScriptStart( const YCPValue& args ) {
    return SET_YCP_CB( CB_ScriptStart, args );
}
/**
 * @builtin CallbackScriptProgress
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>true(boolean ping, string output)</code>. If parameter 'ping' is true than there is no output available, but the script is still running (This functionality enables aborting the script). If it is false, 'output' contains (part of) the script output. The callback function is evaluated when a script is running. Returned 'false' means abort the script.
 * @return void
 */
YCPValue PkgFunctions::CallbackScriptProgress( const YCPValue& args ) {
    return SET_YCP_CB( CB_ScriptProgress, args );
}
/**
 * @builtin CallbackScriptProblem
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string description)</code>. The callback function is evaluated when an error occurrs. Return value 'false' means abort the script.
 * @return void
 */
YCPValue PkgFunctions::CallbackScriptProblem( const YCPValue& args ) {
    return SET_YCP_CB( CB_ScriptProblem, args );
}
/**
 * @builtin CallbackScriptFinish
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the script has been finished.
 * @return void
 */
YCPValue PkgFunctions::CallbackScriptFinish( const YCPValue& args ) {
    return SET_YCP_CB( CB_ScriptFinish, args );
}
/**
 * @builtin CallbackMessage
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string patch_name, string patch_version, string patch_arch, string message)</code>. The callback function is evaluated when a message which is part of a patch should be displayed. Result 'true' means continue, 'false' means abort.
 * @return void
 */
YCPValue PkgFunctions::CallbackMessage( const YCPValue& args ) {
    return SET_YCP_CB( CB_Message, args );
}

/**
 * @builtin CallbackAuthentication
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>map(string url, string message, string username, string password)</code>. The returned map must contain these items: $[ "username" : string, "password" : string, "continue" : boolean ]. If <code>"continue"</code> value is false or is missing the authentification (and the download process) is canceled. The callback function is evaluated when user authentication is required to download the requested file.
 * @return void
 */
YCPValue PkgFunctions::CallbackAuthentication( const YCPValue& args ) {
    return SET_YCP_CB( CB_Authentication, args);
}

/**
 * @builtin CallbackProgressReportStart
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer id, string task, boolean in_percent, boolean is_alive, integer min, integer max, integer val_raw, integer val_percent)</code>. Parameter id is used for callback identification in the Progress() and in the End() callbacks, task describe the action. Parameter in_percent defines whether the progress will be reported in percent, if is_alive is true then the progress will be 'still alive' tick, if both in_percent and is_alive are then a raw value is reported (e.g. number of processed files without knowing the total number).
 * The callback function is evaluated when an progress event starts
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressReportStart(const YCPValue& args)
{
    return SET_YCP_CB( CB_ProgressStart, args);
}

/**
 * @builtin CallbackProgressReportProgress
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(integer id, integer val_raw, integer val_percent)</code>. Parameter id identifies the callback, val_raw is raw status, val_percent is in percent or if the total progress is not known it's -1 (the callback is a 'tick' in this case). If the handler returns false the task is aborted.
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressReportProgress(const YCPValue& args)
{
    return SET_YCP_CB( CB_ProgressProgress, args);
}

/**
 * @builtin CallbackProgressReportEnd
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(integer id)</code>. The id identifies the callback.
 * The callback function is evaluated when an progress event finishes
 * @return void
 */
YCPValue PkgFunctions::CallbackProgressReportEnd(const YCPValue& args)
{
    return SET_YCP_CB( CB_ProgressDone, args);
}

YCPValue PkgFunctions::CallbackInitDownload( const YCPValue& args ) {
    return SET_YCP_CB( CB_InitDownload, args );
}

YCPValue PkgFunctions::CallbackDestDownload( const YCPValue& args ) {
    return SET_YCP_CB( CB_DestDownload, args );
}

YCPValue PkgFunctions::CallbackProcessStart( const YCPValue& args )
{
    return SET_YCP_CB( CB_ProcessStart, args);
}

YCPValue PkgFunctions::CallbackProcessNextStage( const YCPValue& args )
{
    return SET_YCP_CB( CB_ProcessNextStage, args);
}

YCPValue PkgFunctions::CallbackProcessDone( const YCPValue& args )
{
    return SET_YCP_CB( CB_ProcessFinished, args);
}

YCPValue PkgFunctions::CallbackProcessProgress( const YCPValue& args )
{
    return SET_YCP_CB( CB_ProcessProgress, args);
}


/**
 * @builtin CallbackStartRefresh
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>.
 * The callback function is evaluated when repository refresh is started
 * @return void
 */
YCPValue PkgFunctions::CallbackStartRefresh( const YCPValue& args )
{
    return SET_YCP_CB( CB_StartSourceRefresh, args);
}

/**
 * @builtin CallbackDoneRefresh
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>.
 * The callback function is evaluated when repository refresh is finished
 * @return void
 */
YCPValue PkgFunctions::CallbackDoneRefresh( const YCPValue& args )
{
    return SET_YCP_CB( CB_DoneSourceRefresh, args);
}

/**
 * @builtin CallbackFileConflictStart
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>.
 * The callback function is evaluated when file conflict solving is started.
 * @return void
 */
YCPValue PkgFunctions::CallbackFileConflictStart( const YCPValue& args )
{
    return SET_YCP_CB( CB_FileConflictStart, args);
}

/**
 * @builtin CallbackFileConflictProgress
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>bool(integer percent)</code>.
 * The returned boolean indicates whether to continue (true) or abort (false).
 * The callback function is evaluated when file conflict solving is in progress.
 * @return void
 */
YCPValue PkgFunctions::CallbackFileConflictProgress( const YCPValue& args )
{
    return SET_YCP_CB( CB_FileConflictProgress, args);
}

/**
 * @builtin CallbackFileConflictProgress
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback
 * prototype is <code>bool(list&lt;string&gt; excluded_packages, list&lt;string&gt; conflicts)</code>.
 * The returned boolean indicates whether to continue (true) or abort (false).
 * The callback function is evaluated when file conflict solving result is available.
 * @return void
 */
YCPValue PkgFunctions::CallbackFileConflictReport( const YCPValue& args )
{
    return SET_YCP_CB( CB_FileConflictReport, args);
}

/**
 * @builtin CallbackFileConflictFinish
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void()</code>.
 * The callback function is evaluated when file conflict solving is finished.
 * @return void
 */
YCPValue PkgFunctions::CallbackFileConflictFinish( const YCPValue& args )
{
    return SET_YCP_CB( CB_FileConflictFinish, args);
}


#undef SET_YCP_CB
