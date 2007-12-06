
#ifndef Y2PkgFunction_h
#define Y2PkgFunction_h

#include "PkgModuleFunctions.h"

#include <y2/Y2Function.h>

class Y2PkgFunction: public Y2Function
{
    unsigned int m_position;
    PkgModuleFunctions* m_instance;
    YCPValue m_param1;
    YCPValue m_param2;
    YCPValue m_param3;
    YCPValue m_param4;
    YCPValue m_param5;
    string m_name;
public:

    Y2PkgFunction (string name, PkgModuleFunctions* instance, unsigned int pos);
    bool attachParameter (const YCPValue& arg, const int position);
    constTypePtr wantedParameterType () const;
    bool appendParameter (const YCPValue& arg);
    bool finishParameters ();
    YCPValue evaluateCall ();
    bool reset ();
    string name () const;
};

#endif

