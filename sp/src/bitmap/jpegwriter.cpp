//=========== Copyright The VDelta Project, All rights reserved. ==============//
//
// Purpose: I just had to create a JPEG image format helper class because
// either way I would just die when working with JPEG without lower engine
// access ~GabrielV
// 
//=============================================================================//

#include "bitmap/jpegwriter.h"
#include "bitmap/imageformat.h"
#include "jpeglib/jpeglib.h"
#include "tier1/tier1.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

class CUtlBuffer;

namespace JPEGWriter
{
	struct JPEGDestinationManager_t
	{
		struct jpeg_destination_mgr pub;
		CUtlBuffer* pBuffer;
		byte* buffer;
	};

	#define JPEGDestinationManager(cinfo) (JPEGDestinationManager_t*)cinfo->dest

	#define OUTPUT_BUF_SIZE  4096 
	
	static void init_destination(j_compress_ptr cinfo)
	{
		JPEGDestinationManager_t* dest = JPEGDestinationManager(cinfo);

		dest->buffer = (byte*)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(byte));
		dest->pub.next_output_byte = dest->buffer;
		dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
	}

	static boolean empty_output_buffer(j_compress_ptr cinfo)
	{
		JPEGDestinationManager_t* dest = JPEGDestinationManager(cinfo);

		CUtlBuffer* buf = dest->pBuffer;
		buf->Put(dest->buffer, OUTPUT_BUF_SIZE);
		dest->pub.next_output_byte = dest->buffer;
		dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

		return TRUE;
	}

	static void term_destination(j_compress_ptr cinfo)
	{
		JPEGDestinationManager_t* dest = JPEGDestinationManager(cinfo);
		size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

		CUtlBuffer* buf = dest->pBuffer;

		if (datacount > 0)
			buf->Put(dest->buffer, datacount);
	}

	void jpeg_UtlBuffer_dest(j_compress_ptr cinfo, CUtlBuffer* pBuffer)
	{
		JPEGDestinationManager_t* dest;

		if (cinfo->dest == NULL) {
			cinfo->dest = (struct jpeg_destination_mgr*)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
				sizeof(JPEGDestinationManager_t));
		}

		dest = JPEGDestinationManager(cinfo);
		dest->pub.init_destination = init_destination;
		dest->pub.empty_output_buffer = empty_output_buffer;
		dest->pub.term_destination = term_destination;
		dest->pBuffer = pBuffer;
	}

	bool WriteToBuffer(CUtlBuffer& dest, int width, int height, uint8* pImage)
	{
		if (!pImage) { PRINT_STACK("(%s) pImage is incorrect\n", __FUNCTION__); return false; }

		JSAMPROW row_pointer[1];
		int row_stride;

		struct jpeg_error_mgr jerr;
		struct jpeg_compress_struct cinfo;

		row_stride = width * 3;

		cinfo.err = jpeg_std_error(&jerr);

		jpeg_create_compress(&cinfo);
		jpeg_UtlBuffer_dest(&cinfo, &dest);

		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;

		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, 100, TRUE);

		jpeg_start_compress(&cinfo, TRUE);

		while (cinfo.next_scanline < cinfo.image_height)
		{
			row_pointer[0] = &pImage[cinfo.next_scanline * row_stride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}

		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);

		return true;
	}

	bool WriteToFile(const char* path, int width, int height, uint8* pImage)
	{
		if (!pImage) { PRINT_STACK("(%s) pImage is incorrect\n", __FUNCTION__); return false; }

		JSAMPROW row_pointer[1];
		int row_stride;

		struct jpeg_error_mgr jerr;
		struct jpeg_compress_struct cinfo;

		row_stride = width * 3;

		cinfo.err = jpeg_std_error(&jerr);

		jpeg_create_compress(&cinfo);
		CUtlBuffer dest;
		jpeg_UtlBuffer_dest(&cinfo, &dest);

		cinfo.image_width = width;
		cinfo.image_height = height;
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;

		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, 100, TRUE);

		jpeg_start_compress(&cinfo, TRUE);

		while (cinfo.next_scanline < cinfo.image_height)
		{
			row_pointer[0] = &pImage[cinfo.next_scanline * row_stride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}

		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);

		char* szPath = const_cast<char*>(path);
		V_DefaultExtension(szPath, ".jpg", sizeof(szPath));
		V_FixSlashes(szPath);
		if (!g_pFullFileSystem->WriteFile(szPath, "MOD", dest)) {
			Warning("There was an error writing the jpg file: %s\n", szPath);
			return false;
		}

		return true;
	}
} // namespace JPEGWriter