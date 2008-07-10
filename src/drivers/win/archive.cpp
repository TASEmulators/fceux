#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <initguid.h>
#include <ObjBase.h>
#include <OleAuto.h>

#include "7zip/IArchive.h"
#include "file.h"
#include "utils/memorystream.h"
#include "utils/guid.h"

#include "driver.h"
#include "main.h"

DEFINE_GUID(CLSID_CFormat_07,0x23170F69,0x40C1,0x278A,0x10,0x00,0x00,0x01,0x10,0x07,0x00,0x00);

class OutStream : public IArchiveExtractCallback
{
	class SeqStream : public ISequentialOutStream
	{
		uint8* const output;
		uint32 pos;
		const uint32 size;
		ULONG refCount;

		HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID,void**)
		{
			return E_NOINTERFACE;
		}

		HRESULT STDMETHODCALLTYPE Write(const void* data,UInt32 length,UInt32* save)
		{
			if (data != NULL || size == 0)
			{
				//NST_VERIFY( length <= size - pos );

				if (length > size - pos)
					length = size - pos;

				std::memcpy( output + pos, data, length );
				pos += length;

				if (save)
					*save = length;

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}

		ULONG STDMETHODCALLTYPE AddRef()
		{
			return ++refCount;
		}

		ULONG STDMETHODCALLTYPE Release()
		{
			return --refCount;
		}

	public:

		SeqStream(void* d,uint32 s)
		: output(static_cast<uint8*>(d)), pos(0), size(s), refCount(0) {}

		uint32 Size() const
		{
			return pos;
		}
	};

	SeqStream seqStream;
	const uint32 index;
	ULONG refCount;

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID,void**)
	{
		return E_NOINTERFACE;
	}

	HRESULT STDMETHODCALLTYPE PrepareOperation(Int32)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetTotal(UInt64)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetCompleted(const UInt64*)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetOperationResult(Int32)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetStream(UInt32 id,ISequentialOutStream** ptr,Int32 mode)
	{
		switch (mode)
		{
			case NArchive::NExtract::NAskMode::kExtract:
			case NArchive::NExtract::NAskMode::kTest:

				if (id != index || ptr == NULL)
					return S_FALSE;
				else
					*ptr = &seqStream;

			case NArchive::NExtract::NAskMode::kSkip:
				return S_OK;

			default:
				return E_INVALIDARG;
		}
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return ++refCount;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		return --refCount;
	}

public:

	OutStream(uint32 index,void* data,uint32 size)
	: seqStream(data,size), index(index), refCount(0) {}

	uint32 Size() const
	{
		return seqStream.Size();
	}
};

class InStream : public IInStream, private IStreamGetSize
{
	ULONG refCount;

	HRESULT STDMETHODCALLTYPE QueryInterface(REFGUID,void**)
	{
		return E_NOINTERFACE;
	}

	HRESULT STDMETHODCALLTYPE GetSize(UInt64* save)
	{
		if (save)
		{
			*save = size;
			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}

	ULONG STDMETHODCALLTYPE AddRef()
	{
		return ++refCount;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		return --refCount;
	}

protected:

	uint32 size;

public:

	explicit InStream()
	: refCount(0)
	{}
};


class InFileStream : public InStream
{
public:

	~InFileStream()
	{
		if(inf) delete inf;
	}

	std::fstream* inf;

	InFileStream(std::string fname)
	: inf(0)
	{
		inf = FCEUD_UTF8_fstream(fname,"rb");
		if(inf)
		{
			inf->seekg(0,std::ios::end);
			size = inf->tellg();
			inf->seekg(0,std::ios::beg);
		}
	}

	HRESULT STDMETHODCALLTYPE Read(void* data,UInt32 length,UInt32* save)
	{
		if(!inf) return E_FAIL;

		if (data != NULL || length == 0)
		{
			inf->read((char*)data,length);
			length = inf->gcount();
			
			//do we need to do 
			//return E_FAIL;

			if (save)
				*save = length;

			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}

	HRESULT STDMETHODCALLTYPE Seek(Int64 offset,UInt32 origin,UInt64* pos)
	{
		if(!inf) return E_FAIL;

		if (origin < 3)
		{
			std::ios::off_type offtype;
			switch(origin)
			{
			case 0: offtype = std::ios::beg; break;
			case 1: offtype = std::ios::cur; break;
			case 2: offtype = std::ios::end; break;
			default:
				return E_INVALIDARG;
			}
			inf->seekg(offset,offtype);
			origin = inf->tellg();

			if (pos)
				*pos = origin;

			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	
	}
};


class LibRef
{
public:
	HMODULE hmod;
	LibRef(const char* fname) {
		hmod = LoadLibrary(fname);
	}
	~LibRef() {
		if(hmod)
			FreeLibrary(hmod);
	}
};

struct FileSelectorContext
{
	struct Item
	{
		std::string name;
		uint32 size, index;
	};
	std::vector<Item> items;
} *currFileSelectorContext;



static BOOL CALLBACK ArchiveFileSelectorCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			HWND hwndListbox = GetDlgItem(hwndDlg,IDC_LIST1);
			for(uint32 i=0;i<currFileSelectorContext->items.size();i++)
			{
				std::string& name = currFileSelectorContext->items[i].name;
				SendMessage(hwndListbox,LB_ADDSTRING,0,(LPARAM)name.c_str());
			}
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
			case IDC_LIST1:
				if(HIWORD(wParam) == LBN_DBLCLK)
				{
					EndDialog(hwndDlg,SendMessage((HWND)lParam,LB_GETCURSEL,0,0));
				}
				return TRUE;

			case IDOK:
				EndDialog(hwndDlg,SendMessage((HWND)lParam,LB_GETCURSEL,0,0));
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, LB_ERR);
				return TRUE;
		}
		break;
	}
	return FALSE;
}

typedef UINT32 (WINAPI *CreateObjectFunc)(const GUID*,const GUID*,void**);

struct FormatRecord
{
	std::vector<char> signature;
	GUID guid;
};

typedef std::vector<FormatRecord> TFormatRecords;
TFormatRecords formatRecords;
static bool archiveSystemInitialized=false;
static LibRef libref("7z.dll");

void initArchiveSystem()
{
	if(!libref.hmod)
	{
		//couldnt initialize archive system
		return;
	}

	typedef HRESULT (WINAPI *GetNumberOfFormatsFunc)(UINT32 *numFormats);
	typedef HRESULT (WINAPI *GetHandlerProperty2Func)(UInt32 formatIndex, PROPID propID, PROPVARIANT *value);

	GetNumberOfFormatsFunc GetNumberOfFormats = (GetNumberOfFormatsFunc)GetProcAddress(libref.hmod,"GetNumberOfFormats");
	GetHandlerProperty2Func GetHandlerProperty2 = (GetHandlerProperty2Func)GetProcAddress(libref.hmod,"GetHandlerProperty2");

	if(!GetNumberOfFormats || !GetHandlerProperty2)
	{
		//not the right dll.
		return;
	}

	//looks like it is gonna be OK
	archiveSystemInitialized = true;

	UINT32 numFormats;
	GetNumberOfFormats(&numFormats);

	for(uint32 i=0;i<numFormats;i++)
	{
		PROPVARIANT prop;
		prop.vt = VT_EMPTY;

		GetHandlerProperty2(i,NArchive::kStartSignature,&prop);
		
		FormatRecord fr;
		int len = SysStringLen(prop.bstrVal);
		fr.signature.reserve(len);
		for(int j=0;j<len;j++)
			fr.signature.push_back(((char*)prop.bstrVal)[j]);

		GetHandlerProperty2(i,NArchive::kClassID,&prop);
		memcpy((char*)&fr.guid,(char*)prop.bstrVal,16);

		formatRecords.push_back(fr);

		::VariantClear( reinterpret_cast<VARIANTARG*>(&prop) );
	}

}

ArchiveScanRecord FCEUD_ScanArchive(std::string fname)
{
	if(!archiveSystemInitialized)
	{
		return ArchiveScanRecord();
	}

	//check the file against the signatures
	std::fstream* inf = FCEUD_UTF8_fstream(fname,"rb");
	int matchingFormat = -1;
	for(uint32 i=0;i<(int)formatRecords.size();i++)
	{
		inf->seekg(0);
		int size = formatRecords[i].signature.size();
		if(size==0)
			continue; //WHY??
		char* temp = new char[size];
		inf->read((char*)temp,size);
		if(!memcmp(&formatRecords[i].signature[0],temp,size))
		{
			delete[] temp;
			matchingFormat = i;
			break;
		}
		delete[] temp;
	}
	delete inf;

	if(matchingFormat == -1)
		return ArchiveScanRecord();

	CreateObjectFunc CreateObject = (CreateObjectFunc)GetProcAddress(libref.hmod,"CreateObject");
	if(!CreateObject)
		return ArchiveScanRecord();
	IInArchive* object;
	if (!FAILED(CreateObject( &formatRecords[matchingFormat].guid, &IID_IInArchive, (void**)&object )))
	{
		//fetch the start signature
		InFileStream ifs(fname);
		if (SUCCEEDED(object->Open(&ifs,0,0)))
		{
			uint32 numFiles;
			if (SUCCEEDED(object->GetNumberOfItems(&numFiles)))
			{
				object->Release();
				return ArchiveScanRecord(matchingFormat,(int)numFiles);
			}
		}
	}

	object->Release();

	return ArchiveScanRecord();
}

extern HWND hAppWnd;

FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename)
{
	FCEUFILE* fp = 0;
	
	if(!archiveSystemInitialized) {
		MessageBox(hAppWnd,"Could not locate 7z.dll","Failure launching archive browser",0);
		return 0;
	}

	typedef UINT32 (WINAPI *CreateObjectFunc)(const GUID*,const GUID*,void**);
	CreateObjectFunc CreateObject = (CreateObjectFunc)GetProcAddress(libref.hmod,"CreateObject");
	if(!CreateObject)
	{
		MessageBox(hAppWnd,"7z.dll was invalid","Failure launching archive browser",0);
		return 0;
	}


	bool unnamedFileFound = false;
	IInArchive* object;
	if (!FAILED(CreateObject( &formatRecords[asr.type].guid, &IID_IInArchive, (void**)&object )))
	{
		InFileStream ifs(fname);
		if (SUCCEEDED(object->Open(&ifs,0,0)))
		{
			uint32 numFiles;
			if (SUCCEEDED(object->GetNumberOfItems(&numFiles)))
			{
				FileSelectorContext fileSelectorContext;
				currFileSelectorContext = &fileSelectorContext;

				for(uint32 i=0;i<numFiles;i++)
				{
					FileSelectorContext::Item item;
					item.index = i;

					PROPVARIANT prop;
					prop.vt = VT_EMPTY;

					if (FAILED(object->GetProperty( i, kpidSize, &prop )) || prop.vt != VT_UI8 || !prop.uhVal.LowPart || prop.uhVal.HighPart)
					{
						goto bomb;
					}

					item.size = prop.uhVal.LowPart;

					if (FAILED(object->GetProperty( i, kpidPath, &prop )) || prop.vt != VT_BSTR || prop.bstrVal == NULL)
					{
						//mbg 7/10/08 - this was attempting to handle gz files, but it fails later in the extraction
						if(!unnamedFileFound)
						{
							unnamedFileFound = true;
							item.name = "<unnamed>";
						}
						else goto bomb;
					}
					else
					{
						std::wstring tempfname = prop.bstrVal;
						int buflen = tempfname.size()*2;
						char* buf = new char[buflen];
						int ret = WideCharToMultiByte(CP_ACP,0,tempfname.c_str(),tempfname.size(),buf,buflen,0,0);
						if(ret == 0) {
							delete[] buf;
							::VariantClear( reinterpret_cast<VARIANTARG*>(&prop) );
							continue;
						}
						buf[ret] = 0;
						item.name = buf;
						
						delete[] buf;
					}
					
					::VariantClear( reinterpret_cast<VARIANTARG*>(&prop) );
					fileSelectorContext.items.push_back(item);
				}

				//try to load the file directly if we're in autopilot
				int ret = LB_ERR;
				if(innerFilename)
				{
					for(uint32 i=0;i<fileSelectorContext.items.size();i++)
						if(fileSelectorContext.items[i].name == *innerFilename)
						{
							ret = i;
							break;
						}
				}
				else if(numFiles==1)
					//or automatically choose the first file if there was only one file in the archive
					ret = 0;
				else
					//otherwise use the UI
					ret = DialogBoxParam(fceu_hInstance, "ARCHIVECHOOSERDIALOG", hAppWnd, ArchiveFileSelectorCallback, (LPARAM)0);

				if(ret != LB_ERR)
				{
					FileSelectorContext::Item& item = fileSelectorContext.items[ret];
					memorystream* ms = new memorystream(item.size);
					OutStream outStream( item.index, ms->buf(), item.size);
					const uint32 indices[1] = {item.index};
					HRESULT hr = object->Extract(indices,1,0,&outStream);
					if (SUCCEEDED(hr))
					{
						//if we extracted the file correctly
						fp = new FCEUFILE();
						fp->archiveFilename = fname;
						fp->filename = fileSelectorContext.items[ret].name;
						fp->archiveIndex = ret;
						fp->mode = FCEUFILE::READ;
						fp->size = fileSelectorContext.items[ret].size;
						fp->stream = ms;
						fp->archiveCount = (int)numFiles;
					} 
					else
					{
						delete ms;
					}

				} //if returned a file from the fileselector
				
			} //if we scanned the 7z correctly
		} //if we opened the 7z correctly
		bomb:
		object->Release();
	}

	return fp;
}