#pragma once

#include <map>
#include <string>

#include "glm/glm.hpp"

namespace vera {

typedef std::map<std::string,std::string>                   DefinesMap;
typedef std::map<std::string,std::string>::iterator         DefinesMap_it;
typedef std::map<std::string,std::string>::const_iterator   DefinesMap_cit;

class HaveDefines {
public:
    HaveDefines();
    virtual ~HaveDefines();

    virtual void    addDefine( const std::string &_define, int _n );
    virtual void    addDefine( const std::string &_define, float _n );
    virtual void    addDefine( const std::string &_define, double _n );
    virtual void    addDefine( const std::string &_define, glm::vec2 _v );
    virtual void    addDefine( const std::string &_define, glm::vec3 _v );
    virtual void    addDefine( const std::string &_define, glm::vec4 _v );
    virtual void    addDefine( const std::string &_define, float* _value, int _nValues);
    virtual void    addDefine( const std::string &_define, double* _value, int _nValues);
    virtual void    addDefine( const std::string &_define, const std::string &_value = "");
    virtual void    delDefine( const std::string &_define );

    virtual void    mergeDefines( HaveDefines *_haveDefines );
    virtual void    mergeDefines( const HaveDefines *_haveDefines );
    virtual void    mergeDefines( const DefinesMap &_defines );
    virtual void    replaceDefines( const DefinesMap &_defines );
    
    virtual void    printDefines();
    virtual void    addDefinesTo( const std::string &_path );

protected:
    DefinesMap  m_defines;
    bool        m_defineChange;
};

}