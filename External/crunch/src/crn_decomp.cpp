// File: crn_decomp.cpp
// See Copyright Notice and license at the end of inc/crnlib.h

// TODO(co) crunch customization: Do only include the following for shared library builds, not for static builds. Background: Renderer runtime is using "crn_decomp.h>" which already defines certain symbols.
#ifdef _DLL
	#include "crn_core.h"

	// Include the single-file header library with no defines, which brings in the full CRN decompressor.
	#include "../inc/crn_decomp.h"
#endif
