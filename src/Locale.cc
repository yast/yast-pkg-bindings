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

   File:	Locale.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
		Stanislav Visnovsky <visnov@suse.cz>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Namespace:    Pkg
   Summary:	PkgFunctions constructor, destructor and call handling

/-*/


#include "PkgFunctions.h"
#include "log.h"

#include <ycp/YCPList.h>
#include <ycp/YCPString.h>
#include <ycp/YCPVoid.h>

#include <zypp/Locale.h>
#include <zypp/ZConfig.h>
#include <zypp/sat/Pool.h>


/**
 * @builtin SetTextLocale
 * @short Set Package Manager Locale
 * @description
 * Set the given locale as the output locale -- all messages from the package manager (errors, warnings,...)
 * will be returned in the selected language. This built-in does not change package selection in any way.
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgFunctions::SetTextLocale (const YCPString &locale)
{
    try
    {
	zypp::Locale loc = zypp::Locale(locale->value());
	zypp::ZConfig::instance().setTextLocale(loc);
    }
    catch (const std::exception& excpt)
    {
	y2error("Caught an exception: %s", excpt.what());
    }
    catch (...)
    {
	y2internal("Caught an unknown exception");
    }

    return YCPVoid();
}

/**
 * @builtin SetPackageLocale
 * @short Select locale for installation
 * @description
 * Select the main locale for installation, call Pkg::PkgSolve() to select the respective packages.
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgFunctions::SetPackageLocale (const YCPString &locale)
{
    try
    {
	zypp::Locale loc = zypp::Locale(locale->value());

	// add packages for the preferred locale, preserve additional locales
	zypp::LocaleSet lset = zypp::sat::Pool::instance().getRequestedLocales();

	// remove the previous locale
	if (preferred_locale != zypp::Locale::noCode)
	{
	    lset.erase(preferred_locale);
	}

	// add the new locale
	lset.insert(loc);
	zypp::sat::Pool::instance().setRequestedLocales(lset);

	// remember the main locale
	preferred_locale = loc;
    }
    catch(...)
    {
    }

    return YCPVoid();
}

/**
 * @builtin SetLocale
 * @short Set The Main (Preferred) Locale -- OBSOLETED!
 * @description
 * OBSOLETED, DO NOT USE! It has been replaced by SetTextLocale() and SetMainLocale() calls (see bug #223624)
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgFunctions::SetLocale (const YCPString &locale)
{
    y2warning("Pkg::SetLocale() is obsoleted, use Pkg::SetTextLocale() and/or Pkg::SetPackageLocale() instead. Pkg::SetLocale() currently calls both functions");

    SetTextLocale(locale);
    SetPackageLocale(locale);

    return YCPVoid();
}

/**
 * @builtin GetTextLocale
 * @short get the currently preferred locale
 * @return string locale
 * @usage Pkg::GetTextLocale() -> "en_US"
 */
YCPValue
PkgFunctions::GetTextLocale ()
{
    try
    {
	return YCPString(zypp::ZConfig::instance().textLocale().code());
    }
    catch (...)
    {
    }

    return YCPVoid();
}

/**
 * @builtin GetLocale
 * @short get the currently preferred locale
 * @return string locale
 * @usage Pkg::GetLocale () -> "en_US"
 */
YCPValue
PkgFunctions::GetLocale ()
{
    y2warning("Pkg::GetLocale() is obsoleted, use Pkg::GetTextLocale() or Pkg::GetPackageLocale() instead. Pkg::GetLocale() currently calls Pkg::GetTextLocale()");
    return GetTextLocale();
}

/**
 * @builtin GetPackageLocale
 * @short get the locale set by Pkg::SetPackageLocale() call
 * @return string locale
 * @usage Pkg::GetPackageLocale () -> "en_US"
 */
YCPValue
PkgFunctions::GetPackageLocale ()
{
    // the locale hasn't been initialized
    if (preferred_locale == zypp::Locale::noCode)
    {
	y2warning("The package locale hasn't been set, call Pkg::SetPackageLocale() before Pkg::GetPackageLocale()");
    }

    return YCPString(preferred_locale.code());
}

/**
 * @builtin SetAdditionalLocales
 *
 * @short set list of additional locales
 * @description
 * Select additional languages for installation. Call Pkg::Solve() to select the respective packages.
 * @param list<string> locales List of additional locales
 * @return void
 * @usage Pkg::SetAdditionalLocales(["de_DE"]);
 */
YCPValue
PkgFunctions::SetAdditionalLocales (const YCPList &langycplist)
{
    zypp::LocaleSet lset;

    int i = 0;
    while (i < langycplist->size())
    {
	if (langycplist->value(i)->isString())
	{
	    lset.insert(zypp::Locale(langycplist->value(i)->asString()->value()));
	}
	else
	{
	    y2error("Pkg::SetAdditionalLocales ([...,%s,...]) not string", langycplist->value(i)->toString().c_str());
	}
	i++;
    }

    // add the main locale if it's initialized
    if (preferred_locale != zypp::Locale::noCode)
    {
	lset.insert(preferred_locale);
    }

    try
    {
	zypp::sat::Pool::instance().setRequestedLocales(lset);
    }
    catch(...)
    {
    }

    return YCPVoid();
}

/**
 * @builtin GetAdditionalLocales
 *
 * @short return list of additional locales
 * @return list<string>
 * @usage Pkg::GetAdditionalLocales() -> ["de_DE"];
 *
 */
YCPValue
PkgFunctions::GetAdditionalLocales ()
{
    YCPList langycplist;

    try
    {
	zypp::LocaleSet lset = zypp::sat::Pool::instance().getRequestedLocales();

	for (zypp::LocaleSet::const_iterator it = lset.begin();
	     it != lset.end(); ++it)
	{
	  // ignore the main locale
	  if (*it != preferred_locale)
	  {
	    langycplist->add (YCPString(it->code()));
	  }
	}
    }
    catch(...)
    {
    }

    return langycplist;
}

