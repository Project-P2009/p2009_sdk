#pragma once

#include "cbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMessageEntity : public CPointEntity
{
	DECLARE_CLASS(CMessageEntity, CPointEntity);

public:
	void	Spawn(void);
	void	Activate(void);
	void	Think(void);
#ifdef MAPBASE
	virtual
#endif
	void	DrawOverlays(void);

	virtual void UpdateOnRemove();

	void	InputEnable(inputdata_t& inputdata);
	void	InputDisable(inputdata_t& inputdata);
#ifdef MAPBASE
	virtual void	InputSetMessage(inputdata_t& inputdata);
#endif

	void SetMessageText(const string_t& message);

	DECLARE_DATADESC();

protected:
	int				m_radius;
	string_t		m_messageText;
	bool			m_drawText;
	bool			m_bDeveloperOnly;
	bool			m_bEnabled;
};