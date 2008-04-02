

#include "PkgProgress.h"
#include "log.h"

#include <y2/Y2Function.h>

void PkgProgress::Start( const std::string &process, const std::list<std::string> &stages,
    const std::string &help)
{
    if (!running)
    {
	// get the YCP callback handler for destroy event
	Y2Function* ycp_handler = callback_handler._ycpCallbacks.createCallback(PkgFunctions::CallbackHandler::YCPCallbacks::CB_ProcessStart);

	y2debug("ProcessStart");

	// is the callback registered?
	if (ycp_handler != NULL)
	{
	    y2debug("Evaluating ProcessStart callback...");
	    ycp_handler->appendParameter(YCPString(process));

	    // create list of stages
	    YCPList lst;

	    for(std::list<std::string>::const_iterator it = stages.begin();
		it != stages.end() ; ++it )
	    {
		lst->add(YCPString(*it) );
	    }

	    ycp_handler->appendParameter(lst);

	    ycp_handler->appendParameter(YCPString(help));

	    // evaluate the callback function
	    ycp_handler->evaluateCall();
	}

	running = true;

	if (stages.size() > 0)
	{
	    // set the first stage to 'in progress' state
	    NextStage();
	}
    }
    else
    {
	y2error("PkgProgress is already running, ignoring Start()");
    }
}


void PkgProgress::NextStage()
{
    if (running)
    {
	// get the YCP callback handler for destroy event
	Y2Function* ycp_handler = callback_handler._ycpCallbacks.createCallback(PkgFunctions::CallbackHandler::YCPCallbacks::CB_ProcessNextStage);

	// is the callback registered?
	if (ycp_handler != NULL)
	{
	    // evaluate the callback function
	    ycp_handler->evaluateCall();
	}
    }
}

void PkgProgress::Done()
{
    if (running)
    {
	y2debug("ProcessDone");
	// get the YCP callback handler for destroy event
	Y2Function* ycp_handler = callback_handler._ycpCallbacks.createCallback(PkgFunctions::CallbackHandler::YCPCallbacks::CB_ProcessFinished);

	// is the callback registered?
	if (ycp_handler != NULL)
	{
	    y2milestone("Evaluating ProcessDone callback...");
	    // evaluate the callback function
	    ycp_handler->evaluateCall();
	}

	running = false;
    }
}

bool PkgProgress::_receiver(const zypp::ProgressData &progress)
{
    y2milestone("PkgReceiver progress: %lld (%lld%%), running: %s",
	progress.val(), progress.reportValue(), running ? "true" : "false");

    if (running)
    {
	// get the YCP callback handler for destroy event
	Y2Function* ycp_handler = callback_handler._ycpCallbacks.createCallback(PkgFunctions::CallbackHandler::YCPCallbacks::CB_ProcessProgress);

	// is the callback registered?
	if (ycp_handler != NULL)
	{
	    ycp_handler->appendParameter(YCPInteger(progress.reportValue()));
	    // evaluate the callback function
	    YCPValue ret = ycp_handler->evaluateCall();

	    if (!ret.isNull() && ret->isBoolean())
	    {
		return ret->asBoolean()->value();
	    }
	    else
	    {
		y2error("Callback evaluated to a non-boolean value: %s", ret->toString().c_str());
	    }
	}
    }

    return true;
}

PkgProgress::~PkgProgress()
{
    // report done if it hasn't been called
    if (running)
    {
	Done();
    }
}

