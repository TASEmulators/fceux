// RamSearch.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <string>

#include <SDL.h>
#include <QMenuBar>
#include <QAction>
#include <QHeaderView>
#include <QCloseEvent>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QFileDialog>

#include "../../types.h"
#include "../../fceu.h"
#include "../../cheat.h"
#include "../../debug.h"

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RamWatch.h"
#include "Qt/RamSearch.h"
#include "Qt/ConsoleUtilities.h"

static bool ShowROM = false;
static RamSearchDialog_t *ramSearchWin = NULL;
// Too much work to do for resorting the values, and finding the biggest number 
// by sorting in ram list doesn't help too much in usually use, so I gave it up. 
// whitch column does the sort based on, the default status is 0 which means sorted by address
// static int ramSearchSortCol = 0;
// whether it's asc or desc sorting
// static bool ramSearchSortAsc = true;

// used for changing colors of cheated address.
extern int numsubcheats;
extern CHEATF_SUBFAST SubCheats[256];

typedef unsigned int HWAddressType;

//---------------------------------------------------------------------------------------
// Static Prototypes
static void CompactAddrs(void);
static void signal_new_frame (void);
static void SetRamSearchUndoType(int type);
static unsigned int ReadValueAtHardwareAddress(HWAddressType address, unsigned int size);

//---------------------------------------------------------------------------------------

static bool IsHardwareAddressValid(HWAddressType address)
{
	if (!GameInfo)
		return false;

	if(!ShowROM)
		if ((address >= 0x0000 && address < 0x0800) || (address >= 0x6000 && address < 0x8000))
			return true;
		else
			return false;
	else
		if (address >= 0x8000 && address < 0x10000)
			return true;
		else
			return false;
}
#define INVALID_HARDWARE_ADDRESS	((HWAddressType) -1)

struct MemoryRegion
{
	HWAddressType hardwareAddress; // hardware address of the start of this region
	unsigned int size; // number of bytes to the end of this region

	unsigned int virtualIndex; // index into s_prevValues, s_curValues, and s_numChanges, valid after being initialized in ResetMemoryRegions()
	unsigned int itemIndex; // index into listbox items, valid when s_itemIndicesInvalid is false
	unsigned int cheatCount; // how many bytes affected by the cheats. 0 indicates for free, max value is the size.
};

static int MAX_RAM_SIZE = 0;
static unsigned char* s_prevValues = 0; // values at last search or reset
static unsigned char* s_curValues = 0; // values at last frame update
static unsigned short* s_numChanges = 0; // number of changes of the item starting at this virtual index address
static MemoryRegion** s_itemIndexToRegionPointer = 0; // used for random access into the memory list (trading memory size to get speed here, too bad it's so much memory), only valid when s_itemIndicesInvalid is false
static bool s_itemIndicesInvalid = true; // if true, the link from listbox items to memory regions (s_itemIndexToRegionPointer) and the link from memory regions to list box items (MemoryRegion::itemIndex) both need to be recalculated
static bool s_prevValuesNeedUpdate = true; // if true, the "prev" values should be updated using the "cur" values on the next frame update signaled
static unsigned int s_maxItemIndex = 0; // max currently valid item index, the listbox sometimes tries to update things past the end of the list so we need to know this to ignore those attempts

static int disableRamSearchUpdate = false;

// list of contiguous uneliminated memory regions
typedef std::list<MemoryRegion> MemoryList;
static MemoryList s_activeMemoryRegions;
//static CRITICAL_SECTION s_activeMemoryRegionsCS;

// for undo support (could be better, but this way was really easy)
static MemoryList s_activeMemoryRegionsBackup;
static int s_undoType = 0; // 0 means can't undo, 1 means can undo, 2 means can redo

//void RamSearchSaveUndoStateIfNotTooBig(HWND hDlg);
static const int tooManyRegionsForUndo = 10000;

static void ResetMemoryRegions(void)
{
//	Clear_Sound_Buffer();
	fceuWrapperLock();

	s_activeMemoryRegions.clear();

	// use IsHardwareAddressValid to figure out what all the possible memory regions are,
	// split up wherever there's a discontinuity in the address in our software RAM.
	static const int regionSearchGranularity = 1; // if this is too small, we'll waste time (in this function only), but if any region in RAM isn't evenly divisible by this, we might crash.
	HWAddressType hwRegionStart = INVALID_HARDWARE_ADDRESS;
	HWAddressType hwRegionEnd = INVALID_HARDWARE_ADDRESS;
	for(HWAddressType addr = 0; addr != 0x10000+regionSearchGranularity; addr += regionSearchGranularity)
	{
		if (!IsHardwareAddressValid(addr)) {
			// create the region
			if (hwRegionStart != INVALID_HARDWARE_ADDRESS && hwRegionEnd != INVALID_HARDWARE_ADDRESS) {
				MemoryRegion region = { hwRegionStart, regionSearchGranularity + (hwRegionEnd - hwRegionStart) };
				s_activeMemoryRegions.push_back(region);
			}

			hwRegionStart = INVALID_HARDWARE_ADDRESS;
			hwRegionEnd = INVALID_HARDWARE_ADDRESS;
		}
		else {
			if (hwRegionStart != INVALID_HARDWARE_ADDRESS) {
				// continue region
				hwRegionEnd = addr;
			}
			else {
				// start new region
				hwRegionStart = addr;
				hwRegionEnd = addr;
			}
		}
	}


	int nextVirtualIndex = 0;
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
	{
		MemoryRegion& region = *iter;
		region.virtualIndex = nextVirtualIndex;
		nextVirtualIndex = region.virtualIndex + region.size + 4;
	}
	//assert(nextVirtualIndex <= MAX_RAM_SIZE);

	if(nextVirtualIndex > MAX_RAM_SIZE)
	{
		s_prevValues = (unsigned char*)realloc(s_prevValues, sizeof(unsigned char)*nextVirtualIndex);
		memset(s_prevValues, 0, sizeof(unsigned char)*nextVirtualIndex);

		s_curValues = (unsigned char*)realloc(s_curValues, sizeof(unsigned char)*nextVirtualIndex);
		memset(s_curValues, 0, sizeof(unsigned char)*nextVirtualIndex);

		s_numChanges = (unsigned short*)realloc(s_numChanges, sizeof(unsigned short)*nextVirtualIndex);
		memset(s_numChanges, 0, sizeof(unsigned short)*nextVirtualIndex);

		s_itemIndexToRegionPointer = (MemoryRegion**)realloc(s_itemIndexToRegionPointer, sizeof(MemoryRegion*)*nextVirtualIndex);
		memset(s_itemIndexToRegionPointer, 0, sizeof(MemoryRegion*)*nextVirtualIndex);

		MAX_RAM_SIZE = nextVirtualIndex;
	}
	fceuWrapperUnLock();
}

// eliminates a range of hardware addresses from the search results
// returns 2 if it changed the region and moved the iterator to another region
// returns 1 if it changed the region but didn't move the iterator
// returns 0 if it had no effect
// warning: don't call anything that takes an itemIndex in a loop that calls DeactivateRegion...
//   doing so would be tremendously slow because DeactivateRegion invalidates the index cache
static int DeactivateRegion(MemoryRegion& region, MemoryList::iterator& iter, HWAddressType hardwareAddress, unsigned int size)
{
	if(hardwareAddress + size <= region.hardwareAddress || hardwareAddress >= region.hardwareAddress + region.size)
	{
		// region is unaffected
		return 0;
	}
	else if(hardwareAddress > region.hardwareAddress && hardwareAddress + size >= region.hardwareAddress + region.size)
	{
		// erase end of region
		region.size = hardwareAddress - region.hardwareAddress;
		return 1;
	}
	else if(hardwareAddress <= region.hardwareAddress && hardwareAddress + size < region.hardwareAddress + region.size)
	{
		// erase start of region
		int eraseSize = (hardwareAddress + size) - region.hardwareAddress;
		region.hardwareAddress += eraseSize;
		region.size -= eraseSize;
		region.virtualIndex += eraseSize;
		return 1;
	}
	else if(hardwareAddress <= region.hardwareAddress && hardwareAddress + size >= region.hardwareAddress + region.size)
	{
		// erase entire region
		iter = s_activeMemoryRegions.erase(iter);
		s_itemIndicesInvalid = true;
		return 2;
	}
	else //if(hardwareAddress > region.hardwareAddress && hardwareAddress + size < region.hardwareAddress + region.size)
	{
		// split region
		int eraseSize = (hardwareAddress + size) - region.hardwareAddress;
		MemoryRegion region2 = {region.hardwareAddress + eraseSize, region.size - eraseSize, region.virtualIndex + eraseSize};
		region.size = hardwareAddress - region.hardwareAddress;
		iter = s_activeMemoryRegions.insert(++iter, region2);
		s_itemIndicesInvalid = true;
		return 2;
	}
}

/*
// eliminates a range of hardware addresses from the search results
// this is a simpler but usually slower interface for the above function
void DeactivateRegion(HWAddressType hardwareAddress, unsigned int size)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		if(2 != DeactivateRegion(region, iter, hardwareAddress, size))
			++iter;
	}
}
*/

struct AutoCritSect
{
	AutoCritSect() { fceuWrapperLock(); }
	~AutoCritSect() { fceuWrapperUnLock(); }
};

// warning: can be slow
void CalculateItemIndices(int itemSize)
{
	AutoCritSect cs();
	unsigned int itemIndex = 0;
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
	{
		MemoryRegion& region = *iter;
		region.itemIndex = itemIndex;
		int startSkipSize = ((unsigned int)(itemSize - (unsigned int)region.hardwareAddress)) % itemSize; // FIXME: is this still ok?
		unsigned int start = startSkipSize;
		unsigned int end = region.size;
		for (unsigned int i = start; i < end; i += itemSize)
			s_itemIndexToRegionPointer[itemIndex++] = &region;
	}
	s_maxItemIndex = itemIndex;
	s_itemIndicesInvalid = false;
}

template<typename stepType, typename compareType>
void UpdateRegionT(const MemoryRegion& region, const MemoryRegion* nextRegionPtr)
{
	//if(GetAsyncKeyState(VK_SHIFT) & 0x8000) // speed hack
	//	return;

	if(s_prevValuesNeedUpdate)
		memcpy(s_prevValues + region.virtualIndex, s_curValues + region.virtualIndex, region.size + sizeof(compareType) - sizeof(stepType));

	unsigned int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);


	HWAddressType hwSourceAddr = region.hardwareAddress - region.virtualIndex;

	unsigned int indexStart = region.virtualIndex + startSkipSize;
	unsigned int indexEnd = region.virtualIndex + region.size;

	if(sizeof(compareType) == 1)
	{
		for(unsigned int i = indexStart; i < indexEnd; i++)
		{
			if(s_curValues[i] != ReadValueAtHardwareAddress(hwSourceAddr+i, 1)) // if value changed
			{
				s_curValues[i] = ReadValueAtHardwareAddress(hwSourceAddr+i, 1); // update value
				//if(s_numChanges[i] != 0xFFFF)
					s_numChanges[i]++; // increase change count
			}
		}
	}
	else // it's more complicated for non-byte sizes because:
	{    // - more than one byte can affect a given change count entry
	     // - when more than one of those bytes changes simultaneously the entry's change count should only increase by 1
	     // - a few of those bytes can be outside the region

		unsigned int endSkipSize = ((unsigned int)(startSkipSize - region.size)) % sizeof(stepType);
		unsigned int lastIndexToRead = indexEnd + endSkipSize + sizeof(compareType) - sizeof(stepType);
		unsigned int lastIndexToCopy = lastIndexToRead;
		if(nextRegionPtr)
		{
			const MemoryRegion& nextRegion = *nextRegionPtr;
			int nextStartSkipSize = ((unsigned int)(sizeof(stepType) - nextRegion.hardwareAddress)) % sizeof(stepType);
			unsigned int nextIndexStart = nextRegion.virtualIndex + nextStartSkipSize;
			if(lastIndexToCopy > nextIndexStart)
				lastIndexToCopy = nextIndexStart;
		}

		unsigned int nextValidChange [sizeof(compareType)];
		for(unsigned int i = 0; i < sizeof(compareType); i++)
			nextValidChange[i] = indexStart + i;

		for(unsigned int i = indexStart, j = 0; i < lastIndexToRead; i++, j++)
		{
			if(s_curValues[i] != ReadValueAtHardwareAddress(hwSourceAddr+i, 1)) // if value of this byte changed
			{
				if(i < lastIndexToCopy)
					s_curValues[i] = ReadValueAtHardwareAddress(hwSourceAddr+i, 1); // update value
				for(int k = 0; k < sizeof(compareType); k++) // loop through the previous entries that contain this byte
				{
					if(i >= indexEnd+k)
						continue;
					int m = (j-k+sizeof(compareType)) & (sizeof(compareType)-1);
					if(nextValidChange[m] <= i) // if we didn't already increase the change count for this entry
					{
						//if(s_numChanges[i-k] != 0xFFFF)
							s_numChanges[i-k]++; // increase the change count for this entry
						nextValidChange[m] = i-k+sizeof(compareType); // and remember not to increase it again
					}
				}
			}
		}
	}
}

template<typename stepType, typename compareType>
void UpdateRegionsT()
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end();)
	{
		const MemoryRegion& region = *iter;
		++iter;
		const MemoryRegion* nextRegion = (iter == s_activeMemoryRegions.end()) ? NULL : &*iter;

		UpdateRegionT<stepType, compareType>(region, nextRegion);
	}

	s_prevValuesNeedUpdate = false;
}

template<typename stepType, typename compareType>
int CountRegionItemsT()
{
	AutoCritSect cs();
	if(sizeof(stepType) == 1)
	{
		if(s_activeMemoryRegions.empty())
			return 0;

		if(s_itemIndicesInvalid)
			CalculateItemIndices(sizeof(stepType));

		MemoryRegion& lastRegion = s_activeMemoryRegions.back();
		return lastRegion.itemIndex + lastRegion.size;
	}
	else // the branch above is faster but won't work if the step size isn't 1
	{
		int total = 0;
		for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
		{
			MemoryRegion& region = *iter;
			int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
			total += (region.size - startSkipSize + (sizeof(stepType)-1)) / sizeof(stepType);
		}
		return total;
	}
}

// returns information about the item in the form of a "fake" region
// that has the item in it and nothing else
template<typename stepType, typename compareType>
void ItemIndexToVirtualRegion(unsigned int itemIndex, MemoryRegion& virtualRegion)
{
	if(s_itemIndicesInvalid)
		CalculateItemIndices(sizeof(stepType));

	if(itemIndex >= s_maxItemIndex)
	{
		memset(&virtualRegion, 0, sizeof(MemoryRegion));
		return;
	}

	MemoryRegion* region = s_itemIndexToRegionPointer[itemIndex];

	int bytesWithinRegion = (itemIndex - region->itemIndex) * sizeof(stepType);
	int startSkipSize = ((unsigned int)(sizeof(stepType) - region->hardwareAddress)) % sizeof(stepType);
	bytesWithinRegion += startSkipSize;
	
	virtualRegion.size = sizeof(compareType);
	virtualRegion.hardwareAddress = region->hardwareAddress + bytesWithinRegion;
	virtualRegion.virtualIndex = region->virtualIndex + bytesWithinRegion;
	virtualRegion.itemIndex = itemIndex;

	virtualRegion.cheatCount = FCEU_CalcCheatAffectedBytes(virtualRegion.hardwareAddress, virtualRegion.size);
}

template<typename stepType, typename compareType>
unsigned int ItemIndexToVirtualIndex(unsigned int itemIndex)
{
	MemoryRegion virtualRegion;
	ItemIndexToVirtualRegion<stepType,compareType>(itemIndex, virtualRegion);
	return virtualRegion.virtualIndex;
}

template<typename T>
T ReadLocalValue(const unsigned char* data)
{
	return *(const T*)data;
}
//template<> signed char ReadLocalValue(const unsigned char* data) { return *data; }
//template<> unsigned char ReadLocalValue(const unsigned char* data) { return *data; }


template<typename stepType, typename compareType>
compareType GetPrevValueFromVirtualIndex(unsigned int virtualIndex)
{
	return ReadLocalValue<compareType>(s_prevValues + virtualIndex);
	//return *(compareType*)(s_prevValues+virtualIndex);
}
template<typename stepType, typename compareType>
compareType GetCurValueFromVirtualIndex(unsigned int virtualIndex)
{
	return ReadLocalValue<compareType>(s_curValues + virtualIndex);
//	return *(compareType*)(s_curValues+virtualIndex);
}
template<typename stepType, typename compareType>
unsigned short GetNumChangesFromVirtualIndex(unsigned int virtualIndex)
{
	unsigned short num = s_numChanges[virtualIndex];
	//for(unsigned int i = 1; i < sizeof(stepType); i++)
	//	if(num < s_numChanges[virtualIndex+i])
	//		num = s_numChanges[virtualIndex+i];
	return num;
}

template<typename stepType, typename compareType>
compareType GetPrevValueFromItemIndex(unsigned int itemIndex)
{
	int virtualIndex = ItemIndexToVirtualIndex<stepType,compareType>(itemIndex);
	return GetPrevValueFromVirtualIndex<stepType,compareType>(virtualIndex);
}
template<typename stepType, typename compareType>
compareType GetCurValueFromItemIndex(unsigned int itemIndex)
{
	int virtualIndex = ItemIndexToVirtualIndex<stepType,compareType>(itemIndex);
	return GetCurValueFromVirtualIndex<stepType,compareType>(virtualIndex);
}
template<typename stepType, typename compareType>
unsigned short GetNumChangesFromItemIndex(unsigned int itemIndex)
{
	int virtualIndex = ItemIndexToVirtualIndex<stepType,compareType>(itemIndex);
	return GetNumChangesFromVirtualIndex<stepType,compareType>(virtualIndex);
}
template<typename stepType, typename compareType>
unsigned int GetHardwareAddressFromItemIndex(unsigned int itemIndex)
{
	MemoryRegion virtualRegion;
	ItemIndexToVirtualRegion<stepType,compareType>(itemIndex, virtualRegion);
	return virtualRegion.hardwareAddress;
}
template<typename stepType, typename compareType>
unsigned int GetNumCheatsFromIndex(unsigned int itemIndex)
{
	MemoryRegion virtualRegion;
	ItemIndexToVirtualRegion<stepType, compareType>(itemIndex, virtualRegion);
	return virtualRegion.cheatCount;
}

// this one might be unreliable, haven't used it much
template<typename stepType, typename compareType>
unsigned int HardwareAddressToItemIndex(HWAddressType hardwareAddress)
{
	if(s_itemIndicesInvalid)
		CalculateItemIndices(sizeof(stepType));

	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
	{
		MemoryRegion& region = *iter;
		if(hardwareAddress >= region.hardwareAddress && hardwareAddress < region.hardwareAddress + region.size)
		{
			int indexWithinRegion = (hardwareAddress - region.hardwareAddress) / sizeof(stepType);
			return region.itemIndex + indexWithinRegion;
		}
	}

	return -1;
}



// workaround for MSVC 7 that doesn't support varadic C99 macros
#define CALL_WITH_T_SIZE_TYPES_0(functionName, sizeTypeID, isSigned, requiresAligned) \
	(sizeTypeID == 'b' \
		? (isSigned \
			? functionName<char, signed char>() \
			: functionName<char, unsigned char>()) \
	: sizeTypeID == 'w' \
		? (isSigned \
			? (requiresAligned \
				? functionName<short, signed short>() \
				: functionName<char, signed short>()) \
			: (requiresAligned \
				? functionName<short, unsigned short>() \
				: functionName<char, unsigned short>())) \
	: sizeTypeID == 'd' \
		? (isSigned \
			? (requiresAligned \
				? functionName<long, signed long>() \
				: functionName<char, signed long>()) \
			: (requiresAligned \
				? functionName<long, unsigned long>() \
				: functionName<char, unsigned long>())) \
	: functionName<char, signed char>())

#define CALL_WITH_T_SIZE_TYPES_1(functionName, sizeTypeID, isSigned, requiresAligned, p0) \
	(sizeTypeID == 'b' \
		? (isSigned \
			? functionName<char, signed char>(p0) \
			: functionName<char, unsigned char>(p0)) \
	: sizeTypeID == 'w' \
		? (isSigned \
			? (requiresAligned \
				? functionName<short, signed short>(p0) \
				: functionName<char, signed short>(p0)) \
			: (requiresAligned \
				? functionName<short, unsigned short>(p0) \
				: functionName<char, unsigned short>(p0))) \
	: sizeTypeID == 'd' \
		? (isSigned \
			? (requiresAligned \
				? functionName<long, signed long>(p0) \
				: functionName<char, signed long>(p0)) \
			: (requiresAligned \
				? functionName<long, unsigned long>(p0) \
				: functionName<char, unsigned long>(p0))) \
	: functionName<char, signed char>(p0))

#define CALL_WITH_T_SIZE_TYPES_3(functionName, sizeTypeID, isSigned, requiresAligned, p0, p1, p2) \
	(sizeTypeID == 'b' \
		? (isSigned \
			? functionName<char, signed char>(p0, p1, p2) \
			: functionName<char, unsigned char>(p0, p1, p2)) \
	: sizeTypeID == 'w' \
		? (isSigned \
			? (requiresAligned \
				? functionName<short, signed short>(p0, p1, p2) \
				: functionName<char, signed short>(p0, p1, p2)) \
			: (requiresAligned \
				? functionName<short, unsigned short>(p0, p1, p2) \
				: functionName<char, unsigned short>(p0, p1, p2))) \
	: sizeTypeID == 'd' \
		? (isSigned \
			? (requiresAligned \
				? functionName<long, signed long>(p0, p1, p2) \
				: functionName<char, signed long>(p0, p1, p2)) \
			: (requiresAligned \
				? functionName<long, unsigned long>(p0, p1, p2) \
				: functionName<char, unsigned long>(p0, p1, p2))) \
	: functionName<char, signed char>(p0, p1, p2))

#define CALL_WITH_T_SIZE_TYPES_4(functionName, sizeTypeID, isSigned, requiresAligned, p0, p1, p2, p3) \
	(sizeTypeID == 'b' \
		? (isSigned \
			? functionName<char, signed char>(p0, p1, p2, p3) \
			: functionName<char, unsigned char>(p0, p1, p2, p3)) \
	: sizeTypeID == 'w' \
		? (isSigned \
			? (requiresAligned \
				? functionName<short, signed short>(p0, p1, p2, p3) \
				: functionName<char, signed short>(p0, p1, p2, p3)) \
			: (requiresAligned \
				? functionName<short, unsigned short>(p0, p1, p2, p3) \
				: functionName<char, unsigned short>(p0, p1, p2, p3))) \
	: sizeTypeID == 'd' \
		? (isSigned \
			? (requiresAligned \
				? functionName<long, signed long>(p0, p1, p2, p3) \
				: functionName<char, signed long>(p0, p1, p2, p3)) \
			: (requiresAligned \
				? functionName<long, unsigned long>(p0, p1, p2, p3) \
				: functionName<char, unsigned long>(p0, p1, p2, p3))) \
	: functionName<char, signed char>(p0, p1, p2, p3))

// version that takes a forced comparison type
#define CALL_WITH_T_STEP_3(functionName, sizeTypeID, type, requiresAligned, p0, p1, p2) \
	(sizeTypeID == 'b' \
		? functionName<char, type>(p0, p1, p2) \
	: sizeTypeID == 'w' \
		? (requiresAligned \
			? functionName<short, type>(p0, p1, p2) \
			: functionName<char, type>(p0, p1, p2)) \
	: sizeTypeID == 'd' \
		? (requiresAligned \
			? functionName<long, type>(p0, p1, p2) \
			: functionName<char, type>(p0, p1, p2)) \
	: functionName<char, type>(p0, p1, p2))

// version that takes a forced comparison type
#define CALL_WITH_T_STEP_4(functionName, sizeTypeID, type, requiresAligned, p0, p1, p2, p3) \
	(sizeTypeID == 'b' \
		? functionName<char, type>(p0, p1, p2, p3) \
	: sizeTypeID == 'w' \
		? (requiresAligned \
			? functionName<short, type>(p0, p1, p2, p3) \
			: functionName<char, type>(p0, p1, p2, p3)) \
	: sizeTypeID == 'd' \
		? (requiresAligned \
			? functionName<long, type>(p0, p1, p2, p3) \
			: functionName<char, type>(p0, p1, p2, p3)) \
	: functionName<char, type>(p0, p1, p2, p3))

#define CONV_VAL_TO_STR(sizeTypeID, type, val, buf) (sprintf(buf, type == 's' ? "%d" : type == 'u' ? "%u" : type == 'd' ? "%08X" : type == 'w' ? "%04X" : "%02X", sizeTypeID == 'd' ?  (type == 's' ? (long)(val & 0xFFFFFFFF) : (unsigned long)(val & 0xFFFFFFFF)) : sizeTypeID == 'w' ? (type == 's' ? (short)(val & 0xFFFF) : (unsigned short)(val & 0xFFFF)) : (type == 's' ? (char)(val & 0xFF) : (unsigned char)(val & 0xFF))), buf)

#define ConvEditTextNum(hDlg, id, sizeTypeID, type) \
{ \
	BOOL success = false; \
	int val = ReadControlInt(id, type == 'h', success); \
	if (success) \
	{ \
		char num[11]; \
		SetDlgItemText(hDlg, id, CONV_VAL_TO_STR(sizeTypeID, type, val, num)); \
	} else SetDlgItemText(hDlg, id, ""); \
}
// basic comparison functions:
template <typename T> inline bool LessCmp (T x, T y, T i)        { return x < y; }
template <typename T> inline bool MoreCmp (T x, T y, T i)        { return x > y; }
template <typename T> inline bool LessEqualCmp (T x, T y, T i)   { return x <= y; }
template <typename T> inline bool MoreEqualCmp (T x, T y, T i)   { return x >= y; }
template <typename T> inline bool EqualCmp (T x, T y, T i)       { return x == y; }
template <typename T> inline bool UnequalCmp (T x, T y, T i)     { return x != y; }
template <typename T> inline bool DiffByCmp (T x, T y, T p)      { return x - y == p || y - x == p; }
template <typename T> inline bool ModIsCmp (T x, T y, T p)       { return p && x % p == y; }

// compare-to type functions:
template<typename stepType, typename T>
void SearchRelative (bool(*cmpFun)(T,T,T), T ignored, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(GetCurValueFromVirtualIndex<stepType,T>(i), GetPrevValueFromVirtualIndex<stepType,T>(i), param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}
template<typename stepType, typename T>
void SearchSpecific (bool(*cmpFun)(T,T,T), T value, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(GetCurValueFromVirtualIndex<stepType,T>(i), value, param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}
template<typename stepType, typename T>
void SearchAddress (bool(*cmpFun)(T,T,T), T address, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(hwaddr, address, param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}
template<typename stepType, typename T>
void SearchChanges (bool(*cmpFun)(T,T,T), T changes, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(GetNumChangesFromVirtualIndex<stepType,T>(i), changes, param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}

static char rs_c='s';
static char rs_o='=';
static char rs_t='s';
static int rs_param=0, rs_val=0, rs_val_valid=0;
static char rs_type_size = 'b', rs_last_type_size = rs_type_size;
static bool noMisalign = true, rs_last_no_misalign = noMisalign;
//static bool littleEndian = false;
static int last_rs_possible = -1;
static int last_rs_regions = -1;

static void prune(char c,char o,char t,int v,int p)
{
	// repetition-reducing macros
	#define DO_SEARCH(sf) \
	switch (o) \
	{ \
		case '<': DO_SEARCH_2(LessCmp,sf); break; \
		case '>': DO_SEARCH_2(MoreCmp,sf); break; \
		case '=': DO_SEARCH_2(EqualCmp,sf); break; \
		case '!': DO_SEARCH_2(UnequalCmp,sf); break; \
		case 'l': DO_SEARCH_2(LessEqualCmp,sf); break; \
		case 'm': DO_SEARCH_2(MoreEqualCmp,sf); break; \
		case 'd': DO_SEARCH_2(DiffByCmp,sf); break; \
		case '%': DO_SEARCH_2(ModIsCmp,sf); break; \
		default: assert(!"Invalid operator for this search type."); break; \
	}

	// perform the search, eliminating nonmatching values
	switch (c)
	{
		#define DO_SEARCH_2(CmpFun,sf) CALL_WITH_T_SIZE_TYPES_3(sf, rs_type_size, t, noMisalign, CmpFun,v,p)
		case 'r': DO_SEARCH(SearchRelative); break;
		case 's': DO_SEARCH(SearchSpecific); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) CALL_WITH_T_STEP_3(sf, rs_type_size, unsigned int, noMisalign, CmpFun,v,p)
		case 'a': DO_SEARCH(SearchAddress); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) CALL_WITH_T_STEP_3(sf, rs_type_size, unsigned short, noMisalign, CmpFun,v,p)
		case 'n': DO_SEARCH(SearchChanges); break;

		default: assert(!"Invalid search comparison type."); break;
	}

	s_prevValuesNeedUpdate = true;

	int prevNumItems = last_rs_possible;

	CompactAddrs();

	if(prevNumItems == last_rs_possible)
	{
		SetRamSearchUndoType(0); // nothing to undo
	}
}




template<typename stepType, typename T>
bool CompareRelativeAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T ignored, T param)
{
	return cmpFun(GetCurValueFromItemIndex<stepType,T>(itemIndex), GetPrevValueFromItemIndex<stepType,T>(itemIndex), param);
}
template<typename stepType, typename T>
bool CompareSpecificAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T value, T param)
{
	return cmpFun(GetCurValueFromItemIndex<stepType,T>(itemIndex), value, param);
}
template<typename stepType, typename T>
bool CompareAddressAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T address, T param)
{
	return cmpFun(GetHardwareAddressFromItemIndex<stepType,T>(itemIndex), address, param);
}
template<typename stepType, typename T>
bool CompareChangesAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T changes, T param)
{
	return cmpFun(GetNumChangesFromItemIndex<stepType,T>(itemIndex), changes, param);
}

static int cmpVal  = 0;
static int cmpAddr = 0;
static int cmpChgs = 0;
static int diffByVal = 0;
static int moduloVal = 0;

static bool Set_RS_Val(void)
{
	bool success = true;

	// update rs_val
	switch(rs_c)
	{
		case 'r':
		default:
			rs_val = 0;
			break;
		case 's':
			rs_val = cmpVal; success = true;
			if(!success)
				return false;
			if((rs_type_size == 'b' && rs_t == 's' && (rs_val < -128 || rs_val > 127)) ||
			   (rs_type_size == 'b' && rs_t != 's' && (rs_val < 0 || rs_val > 255)) ||
			   (rs_type_size == 'w' && rs_t == 's' && (rs_val < -32768 || rs_val > 32767)) ||
			   (rs_type_size == 'w' && rs_t != 's' && (rs_val < 0 || rs_val > 65535)))
			   return false;
			break;
		case 'a':
			rs_val = cmpAddr; success = true;
			if(!success || rs_val < 0 || rs_val > 0x06040000)
				return false;
			break;
		case 'n': {
			rs_val = cmpChgs; success = true;
			if(!success || rs_val < 0 || rs_val > 0xFFFF)
				return false;
		}	break;
	}

	// also update rs_param
	switch(rs_o)
	{
		default:
			rs_param = 0;
			break;
		case 'd':
			rs_param = diffByVal; success = true;
			if(!success)
				return false;
			if(rs_param < 0)
				rs_param = -rs_param;
			break;
		case '%':
			rs_param = moduloVal; success = true;
			if(!success || rs_param == 0)
				return false;
			break;
	}

	// validate that rs_param fits in the comparison data type
	{
		int appliedSize = rs_type_size;
		int appliedSign = rs_t;
		if(rs_c == 'n')
			appliedSize = 'w', appliedSign = 'u';
		if(rs_c == 'a')
			appliedSize = 'd', appliedSign = 'u';
		if((appliedSize == 'b' && appliedSign == 's' && (rs_param < -128 || rs_param > 127)) ||
		   (appliedSize == 'b' && appliedSign != 's' && (rs_param < 0 || rs_param > 255)) ||
		   (appliedSize == 'w' && appliedSign == 's' && (rs_param < -32768 || rs_param > 32767)) ||
		   (appliedSize == 'w' && appliedSign != 's' && (rs_param < 0 || rs_param > 65535)))
		   return false;
	}

	return true;
}

static bool IsSatisfied(int itemIndex)
{
	if(!rs_val_valid)
		return true;
	int o = rs_o;
	switch (rs_c)
	{
		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) return CALL_WITH_T_SIZE_TYPES_4(sf, rs_type_size,(rs_t=='s'),noMisalign, CmpFun,itemIndex,rs_val,rs_param);
		case 'r': DO_SEARCH(CompareRelativeAtItem); break;
		case 's': DO_SEARCH(CompareSpecificAtItem); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) return CALL_WITH_T_STEP_4(sf, rs_type_size, unsigned int, noMisalign, CmpFun,itemIndex,rs_val,rs_param);
		case 'a': DO_SEARCH(CompareAddressAtItem); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) return CALL_WITH_T_STEP_4(sf, rs_type_size, unsigned short, noMisalign, CmpFun,itemIndex,rs_val,rs_param);
		case 'n': DO_SEARCH(CompareChangesAtItem); break;
	}
	return false;
}



static unsigned int ReadValueAtSoftwareAddress(const unsigned char* address, unsigned int size)
{
	unsigned int value = 0;
	//if(!byteSwapped)
	{
		// assumes we're little-endian
		memcpy(&value, address, size);
	}
	return value;
}
static void WriteValueAtSoftwareAddress(unsigned char* address, unsigned int value, unsigned int size)
{
	//if(!byteSwapped)
	{
		// assumes we're little-endian
		memcpy(address, &value, size);
	}
}
static unsigned int ReadValueAtHardwareAddress(HWAddressType address, unsigned int size)
{
	unsigned int value = 0;

	// read as little endian
	for(unsigned int i = 0; i < size; i++)
	{
		value <<= 8;
		value |= (IsHardwareAddressValid(address) ? GetMem(address) : 0);
		address++;
	}
	return value;
}
static bool WriteValueAtHardwareAddress(HWAddressType address, unsigned int value, unsigned int size)
{
	// TODO: NYI
	return false;
}



static int ResultCount=0;
static bool AutoSearch=false;
static bool AutoSearchAutoRetry=false;
static void UpdatePossibilities(int rs_possible, int regions);


static void CompactAddrs(void)
{
	int size = (rs_type_size=='b' || !noMisalign) ? 1 : 2;
	int prevResultCount = ResultCount;

	CalculateItemIndices(size);
	ResultCount = CALL_WITH_T_SIZE_TYPES_0(CountRegionItemsT, rs_type_size,rs_t=='s',noMisalign);

	UpdatePossibilities(ResultCount, (int)s_activeMemoryRegions.size());

	// FIXME
	//if(ResultCount != prevResultCount)
	//	ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),ResultCount);
}

static void soft_reset_address_info (void)
{
	/*
	if (resetPrevValues) {
		if(s_prevValues)
			memcpy(s_prevValues, s_curValues, (sizeof(*s_prevValues)*(MAX_RAM_SIZE)));
		s_prevValuesNeedUpdate = false;
	}*/
	s_prevValuesNeedUpdate = false;
	ResetMemoryRegions();
//	UpdateMemoryCheatStatus();
	if(ramSearchWin == NULL)
	{
		fceuWrapperLock();
		s_activeMemoryRegions.clear();
		fceuWrapperUnLock();
		ResultCount = 0;
	}
	else
	{
		// force s_prevValues to be valid
		signal_new_frame();
		s_prevValuesNeedUpdate = true;
		signal_new_frame();
	}
	if(s_numChanges)
		memset(s_numChanges, 0, (sizeof(*s_numChanges)*(MAX_RAM_SIZE)));
	CompactAddrs();
}
static void reset_address_info (void)
{
	SetRamSearchUndoType(0);
	fceuWrapperLock();
	s_activeMemoryRegionsBackup.clear(); // not necessary, but we'll take the time hit here instead of at the next thing that sets up an undo
	fceuWrapperUnLock();
	if(s_prevValues)
		memcpy(s_prevValues, s_curValues, (sizeof(*s_prevValues)*(MAX_RAM_SIZE)));
	s_prevValuesNeedUpdate = false;
	ResetMemoryRegions();
//	UpdateMemoryCheatStatus();
	if (ramSearchWin == NULL)
	{
		fceuWrapperLock();
		s_activeMemoryRegions.clear();
		fceuWrapperUnLock();
		ResultCount = 0;
	}
	else
	{
		// force s_prevValues to be valid
		signal_new_frame();
		s_prevValuesNeedUpdate = true;
		signal_new_frame();
	}
	memset(s_numChanges, 0, (sizeof(*s_numChanges)*(MAX_RAM_SIZE)));
	CompactAddrs();
}

static void signal_new_frame (void)
{
	fceuWrapperLock();
	CALL_WITH_T_SIZE_TYPES_0(UpdateRegionsT, rs_type_size, rs_t=='s', noMisalign);
	fceuWrapperUnLock();
}

static void SetRamSearchUndoType(int type)
{
	if (s_undoType != type)
	{
		//if((s_undoType!=2 && s_undoType!=-1)!=(type!=2 && type!=-1))
		//	SendDlgItemMessage(hDlg,IDC_C_UNDO,WM_SETTEXT,0,(LPARAM)((type == 2 || type == -1) ? "Redo" : "Undo"));
		//if((s_undoType>0)!=(type>0))
		//	EnableWindow(GetDlgItem(hDlg,IDC_C_UNDO),type>0);
		s_undoType = type;
	}
}

static void UpdateRamSearchTitleBar(int percent = -1)
{
	char Str_Tmp[128];
#define HEADER_STR " RAM Search - "
#define PROGRESS_STR " %d%% ... "
#define STATUS_STR "%d Possibilit%s (%d Region%s)"

	int poss = last_rs_possible;
	int regions = last_rs_regions;
	if (poss <= 0)
	{
		strcpy(Str_Tmp," RAM Search");
	}
	else if (percent <= 0)
	{
		sprintf(Str_Tmp, HEADER_STR STATUS_STR, poss, poss==1?"y":"ies", regions, regions==1?"":"s");
	}
	else
	{
		sprintf(Str_Tmp, PROGRESS_STR STATUS_STR, percent, poss, poss==1?"y":"ies", regions, regions==1?"":"s");
	}

	if ( ramSearchWin )
	{
		ramSearchWin->setWindowTitle("RAM Search");
	}
}

static void UpdatePossibilities(int rs_possible, int regions)
{
	if (rs_possible != last_rs_possible)
	{
		last_rs_possible = rs_possible;
		last_rs_regions = regions;
		UpdateRamSearchTitleBar();
	}
}

//----------------------------------------------------------------------------
void openRamSearchWindow( QWidget *parent )
{
	if ( ramSearchWin != NULL )
	{
		return;
	}
   ramSearchWin = new RamSearchDialog_t(parent);
	
   ramSearchWin->show();
}

//----------------------------------------------------------------------------
RamSearchDialog_t::RamSearchDialog_t(QWidget *parent)
	: QDialog( parent )
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox, *hbox1, *hbox2, *hbox3;
	QVBoxLayout *vbox, *vbox1, *vbox2;
	QTreeWidgetItem *item;
	QGridLayout *grid;
	QGroupBox *frame;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    fontCharWidth = 2 * fm.horizontalAdvance(QLatin1Char('2'));
#else
    fontCharWidth = 2 * fm.width(QLatin1Char('2'));
#endif

	setWindowTitle("RAM Search");

	resize( 512, 512 );

	mainLayout = new QVBoxLayout();
	hbox1      = new QHBoxLayout();

	mainLayout->addLayout( hbox1 );

	tree = new QTreeWidget();

	tree->setColumnCount(4);

	item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Address" ) );
	item->setText( 1, QString::fromStdString( "Value" ) );
	item->setText( 2, QString::fromStdString( "Previous" ) );
	item->setText( 3, QString::fromStdString( "Changes" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);
	item->setTextAlignment( 2, Qt::AlignLeft);
	item->setTextAlignment( 3, Qt::AlignLeft);

	//connect( tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
	//		   this, SLOT(watchClicked( QTreeWidgetItem*, int)) );

	tree->setHeaderItem( item );

	//tree->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

	vbox  = new QVBoxLayout();
	hbox1->addWidget( tree  );
	hbox1->addLayout( vbox  );

	searchButton = new QPushButton( tr("Search") );
	vbox->addWidget( searchButton );
	//connect( searchButton, SIGNAL(clicked(void)), this, SLOT(moveWatchUpClicked(void)));
	//searchButton->setEnabled(false);

	resetButton = new QPushButton( tr("Reset") );
	vbox->addWidget( resetButton );
	//connect( resetButton, SIGNAL(clicked(void)), this, SLOT(moveWatchDownClicked(void)));
	//down_btn->setEnabled(false);

	clearChangeButton = new QPushButton( tr("Clear Change") );
	vbox->addWidget( clearChangeButton );
	//connect( clearChangeButton, SIGNAL(clicked(void)), this, SLOT(editWatchClicked(void)));
	//clearChangeButton->setEnabled(false);

	undoButton = new QPushButton( tr("Remove") );
	vbox->addWidget( undoButton );
	//connect( undoButton, SIGNAL(clicked(void)), this, SLOT(removeWatchClicked(void)));
	//undoButton->setEnabled(false);
	
	searchROMCbox = new QCheckBox( tr("Search ROM") );
	vbox->addWidget( searchROMCbox );
	//connect( undoButton, SIGNAL(clicked(void)), this, SLOT(removeWatchClicked(void)));
	//undoButton->setEnabled(false);

	elimButton = new QPushButton( tr("Eliminate") );
	vbox->addWidget( elimButton );
	//connect( elimButton, SIGNAL(clicked(void)), this, SLOT(newWatchClicked(void)));
	//elimButton->setEnabled(false);

	watchButton = new QPushButton( tr("Watch") );
	vbox->addWidget( watchButton );
	//connect( watchButton, SIGNAL(clicked(void)), this, SLOT(dupWatchClicked(void)));
	watchButton->setEnabled(false);

	addCheatButton = new QPushButton( tr("Add Cheat") );
	vbox->addWidget( addCheatButton );
	//connect( addCheatButton, SIGNAL(clicked(void)), this, SLOT(sepWatchClicked(void)));
	addCheatButton->setEnabled(false);

	hexEditButton = new QPushButton( tr("Hex Editor") );
	vbox->addWidget( hexEditButton );
	//connect( hexEditButton, SIGNAL(clicked(void)), this, SLOT(sepWatchClicked(void)));
	hexEditButton->setEnabled(false);

	hbox2 = new QHBoxLayout();
	mainLayout->addLayout( hbox2 );
	frame = new QGroupBox( tr("Comparison Operator") );
	vbox  = new QVBoxLayout();

	hbox2->addWidget( frame );
	frame->setLayout(vbox);

	lt_btn = new QRadioButton( tr("Less Than") );
	gt_btn = new QRadioButton( tr("Greater Than") );
	le_btn = new QRadioButton( tr("Less Than or Equal To") );
	ge_btn = new QRadioButton( tr("Greater Than or Equal To") );
	eq_btn = new QRadioButton( tr("Equal To") );
	ne_btn = new QRadioButton( tr("Not Equal To") );
	df_btn = new QRadioButton( tr("Different By:") );
	md_btn = new QRadioButton( tr("Modulo") );

	eq_btn->setChecked(true);

	diffByEdit = new QLineEdit();
	moduloEdit = new QLineEdit();

	vbox->addWidget( lt_btn );
	vbox->addWidget( gt_btn );
	vbox->addWidget( le_btn );
	vbox->addWidget( ge_btn );
	vbox->addWidget( eq_btn );
	vbox->addWidget( ne_btn );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( df_btn );
	hbox->addWidget( diffByEdit );

	hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->addWidget( md_btn );
	hbox->addWidget( moduloEdit );

	vbox1 = new QVBoxLayout();
	grid  = new QGridLayout();
	hbox2->addLayout( vbox1 );
	frame = new QGroupBox( tr("Compare To/By") );
	frame->setLayout( grid );
	vbox1->addWidget( frame );

	pv_btn = new QRadioButton( tr("Previous Value") );
	sv_btn = new QRadioButton( tr("Specific Value:") );
	sa_btn = new QRadioButton( tr("Specific Address:") );
	nc_btn = new QRadioButton( tr("Number of Changes:") );

	pv_btn->setChecked(true);

	specValEdit   = new QLineEdit();
	specAddrEdit  = new QLineEdit();
	numChangeEdit = new QLineEdit();

	grid->addWidget( pv_btn       , 0, 0, Qt::AlignLeft );
	grid->addWidget( sv_btn       , 1, 0, Qt::AlignLeft );
	grid->addWidget( specValEdit  , 1, 1, Qt::AlignLeft );
	grid->addWidget( sa_btn       , 2, 0, Qt::AlignLeft );
	grid->addWidget( specAddrEdit , 2, 1, Qt::AlignLeft );
	grid->addWidget( nc_btn       , 3, 0, Qt::AlignLeft );
	grid->addWidget( numChangeEdit, 3, 1, Qt::AlignLeft );

	vbox  = new QVBoxLayout();
	hbox3 = new QHBoxLayout();
	frame = new QGroupBox( tr("Data Size") );
	frame->setLayout( vbox );
	vbox1->addLayout( hbox3 );
	hbox3->addWidget( frame );

	ds1_btn = new QRadioButton( tr("1 Byte") );
	ds2_btn = new QRadioButton( tr("2 Byte") );
	ds4_btn = new QRadioButton( tr("4 Byte") );
	misalignedCbox = new QCheckBox( tr("Check Misaligned") );
	misalignedCbox->setEnabled(false);

	ds1_btn->setChecked(true);

	vbox->addWidget( ds1_btn );
	vbox->addWidget( ds2_btn );
	vbox->addWidget( ds4_btn );
	vbox->addWidget( misalignedCbox );

	vbox  = new QVBoxLayout();
	vbox2 = new QVBoxLayout();
	frame = new QGroupBox( tr("Data Type / Display") );
	frame->setLayout( vbox );
	vbox2->addWidget( frame );
	hbox3->addLayout( vbox2 );

	signed_btn   = new QRadioButton( tr("Signed") );
	unsigned_btn = new QRadioButton( tr("Unsigned") );
	hex_btn      = new QRadioButton( tr("Hexadecimal") );

	vbox->addWidget( signed_btn );
	vbox->addWidget( unsigned_btn );
	vbox->addWidget( hex_btn );
	signed_btn->setChecked(true);

	autoSearchCbox = new QCheckBox( tr("Auto-Search") );
	autoSearchCbox->setEnabled(true);
	vbox2->addWidget( autoSearchCbox );

	setLayout( mainLayout );

	updateTimer  = new QTimer( this );

   connect( updateTimer, &QTimer::timeout, this, &RamSearchDialog_t::periodicUpdate );

	updateTimer->start( 100 ); // 10hz
}
//----------------------------------------------------------------------------
RamSearchDialog_t::~RamSearchDialog_t(void)
{
	updateTimer->stop();
	printf("Destroy RAM Watch Config Window\n");
	ramSearchWin = NULL;
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::closeEvent(QCloseEvent *event)
{
   printf("RAM Watch Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void RamSearchDialog_t::periodicUpdate(void)
{
	bool buttonEnable;
	QTreeWidgetItem *item;

	item = tree->currentItem();

	if ( item == NULL )
	{
		buttonEnable = false;
	}
	else
	{
		buttonEnable = true;
	}
}
//----------------------------------------------------------------------------
