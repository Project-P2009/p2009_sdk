#ifndef PROPLASERCATCHER_H
#define PROPLASERCATCHER_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"
#include "props.h"				// CDynamicProp base class

class CPropLaserCatcher : public CDynamicProp
{
	DECLARE_CLASS( CPropLaserCatcher, CDynamicProp );
    public:
    void Spawn( void );
    void Precache( void );
    bool KeyValue( const char *szKeyName, const char *szValue );

    void TouchThink( void );

	DECLARE_DATADESC();
    protected:
    COutputEvent m_OnPowered;
    COutputEvent m_OnUnpowered;
};

#endif // PROPLASERCATCHER_H