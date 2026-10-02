#pragma once

namespace MapPreviewFix
{
	// The map preview buffer (CMapData::MapPreviewData) used to be a fixed 0x40000 array,
	// and all the instructions that refer to it are rewritten to point to a buffer whose
	// size is decided by the map that is currently loaded.
	//
	// Call this once the size of the map is known (loading / creating / resizing a map):
	// it reallocates the buffer if it is too small, and rewrites every instruction that
	// refers to the buffer so that they point to the new address.
	void UpdateBuffer(int width, int height);
}
