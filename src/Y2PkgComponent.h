
#ifndef Y2PkgComponent_h
#define Y2PkgComponent_h

#include <y2/Y2Namespace.h>
#include <y2/Y2Component.h>

class Y2PkgComponent : public Y2Component {
public:
    virtual Y2Namespace *import (const char* name);

    virtual string name () const { return "Pkg";}
    
    static Y2PkgComponent* instance();

    static void destroy();

    ~Y2PkgComponent();
    
private:
    static Y2PkgComponent* m_instance;
};

#endif
