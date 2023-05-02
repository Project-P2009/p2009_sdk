//=========== Copyright The VDelta Project, All rights reserved. ==============//
//
// Purpose: I just had to create a JPEG image format helper class because
// either way I would just die when working with JPEG without lower engine
// access ~GabrielV
// 
//=============================================================================//

#ifndef JPEGWRITER_H
#define JPEGWRITER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"

class CUtlBuffer;

namespace JPEGWriter
{

	bool WriteToBuffer(CUtlBuffer& dest, int width, int height, uint8* pImage);
	bool WriteToFile(const char* path, int width, int height, uint8* pImage);

} // namespace JPEGWriter

#endif //JPEGWRITER_H