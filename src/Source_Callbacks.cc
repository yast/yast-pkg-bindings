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
   Summary:     Callbacks functions related to repository registration
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>

/*
  Textdomain "pkg-bindings"
*/

void PkgModuleFunctions::CallSourceReportStart(const std::string &text)
{
    // get the YCP callback handler
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportStart);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// add parameters
	ycp_handler->appendParameter( YCPInteger(0LL) );
	ycp_handler->appendParameter( YCPString("") );
	ycp_handler->appendParameter( YCPString(text) );
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportEnd(const std::string &text)
{
    // get the YCP callback handler for end event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportEnd);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// add parameters
	ycp_handler->appendParameter( YCPInteger(0LL) );
	ycp_handler->appendParameter( YCPString("") );
	ycp_handler->appendParameter( YCPString(text) );
	ycp_handler->appendParameter( YCPString("NO_ERROR") );
	ycp_handler->appendParameter( YCPString("") );
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportInit()
{
    // get the YCP callback handler for init event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportInit);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportDestroy()
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportDestroy);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallInitDownload(const std::string &task)
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_InitDownload);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	ycp_handler->appendParameter(YCPString(task));
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallDestDownload()
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_DestDownload);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

// this method should be used instead of RepoManager::refreshMetadata()
void PkgModuleFunctions::RefreshWithCallbacks(const zypp::RepoInfo &repo, const zypp::ProgressData::ReceiverFnc &progressrcv)
{
    CallInitDownload(std::string(_("Refreshing repository ") + repo.alias()));

    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	repomanager.refreshMetadata(repo, zypp::RepoManager::RefreshIfNeeded, progressrcv);
    }
    catch(...)
    {
	// call the final event even in case of exception
	CallDestDownload();
	// rethrow the execption
	throw;
    }

    CallDestDownload();
}

// this method should be used instead of RepoManager::probe()
zypp::repo::RepoType PkgModuleFunctions::ProbeWithCallbacks(const zypp::Url &url)
{
    CallInitDownload(std::string(_("Probing repository ") + url.asString()));

    zypp::repo::RepoType repotype;

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    _silent_probing = ZyppRecipients::MEDIA_CHANGE_DISABLE;

    try
    {
	// probe type of the repository 
	zypp::RepoManager repomanager = CreateRepoManager();
	repotype = repomanager.probe(url);
    }
    catch(...)
    {
	// call the final event even in case of exception
	CallDestDownload();

	// restore the probing flag
	_silent_probing = _silent_probing_old;

	// rethrow the execption
	throw;
    }

    CallDestDownload();

    // restore the probing flag
    _silent_probing = _silent_probing_old;

    return repotype;
}

