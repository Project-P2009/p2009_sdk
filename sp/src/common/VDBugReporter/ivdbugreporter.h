//=========== Copyright The VDelta Project, All rights reserved. ==============//
//
// Purpose: 
//
// $NoKeywords: $
//============================================================================//
#ifndef IVDBUGREPORTER_H
#define IVDBUGREPORTER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include "mathlib/vector.h"

typedef struct BugLocalization_s
{
	Vector m_vecPosition;
	QAngle m_angOrientation;
} BugLocalization_t;

abstract_class IVDBugReporter : public IBaseInterface
{
public:
	virtual bool Init(CreateInterfaceFn factory) = 0;
	virtual void Shutdown() = 0;

	virtual void SetMapname(const char* pszMapname) = 0;

	// Submission API
	virtual void StartBugReport() = 0;
	virtual void CancelBugReport() = 0;
	virtual void SubmitBugReport() = 0;

	virtual void SetTitle(char const* title) = 0;
	virtual void SetDescription(char const* desc) = 0;

	virtual void SetSeverity(int severity) = 0;
	virtual void SetCategory(int category) = 0;
	virtual void SetLocalization(BugLocalization_t* pLocalization) = 0;
};

#define INTERFACEVERSION_VDBUGREPORTER "VDBugReporter001"

#define VDBugRep_Log(...) ConColorMsg(Color(119, 27, 247, 255), "[VDBugReporter] " __VA_ARGS__)
#define VDBugRep_Err(...) ConColorMsg(Color(247, 27, 27, 255), "[VDBugReporter] " __VA_ARGS__)

#endif