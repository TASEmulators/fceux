#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <initguid.h>
#include <ObjBase.h>
#include <OleAuto.h>

#include "7zip/IArchive.h"

#include "driver.h"
#include "main.h"

DEFINE_GUID(CLSID_CFormat7z,0x23170F69,0x40C1,0x278A,0x10,0x00,0x00,0x01,0x10,0x07,0x00,0x00);

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
		hmod = LoadLibrary("7zxa.dll");
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

void do7zip(HWND hParent, std::string fname)
{
	LibRef libref("7zxa.dll");
	if(!libref.hmod) {
		MessageBox(hParent,"Could not locate 7zxa.dll","Failure launching 7z archive browser",0);
		return;
	}

	typedef UINT32 (WINAPI *CreateObjectFunc)(const GUID*,const GUID*,void**);
	CreateObjectFunc CreateObject = (CreateObjectFunc)GetProcAddress(libref.hmod,"CreateObject");
	if(!CreateObject)
	{
		MessageBox(hParent,"7zxa.dll was invalid","Failure launching 7z archive browser",0);
		return;
	}

	IInArchive* object;
	if (!FAILED(CreateObject( &CLSID_CFormat7z, &IID_IInArchive, (void**)&object )))
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
						continue;

					item.size = prop.uhVal.LowPart;

					if (FAILED(object->GetProperty( i, kpidPath, &prop )) || prop.vt != VT_BSTR || prop.bstrVal == NULL)
						continue;


					std::wstring tempfname = prop.bstrVal;
					int buflen = tempfname.size()*2;
					char* buf = new char[buflen];
					int ret = WideCharToMultiByte(CP_ACP,0,tempfname.c_str(),tempfname.size(),buf,buflen,0,0);
					if(ret == 0) {
						delete[] buf;
						continue;
					}
					buf[ret] = 0;
					item.name = buf;
					delete[] buf;

					::VariantClear( reinterpret_cast<VARIANTARG*>(&prop) );

					fileSelectorContext.items.push_back(item);
				}

				int ret = DialogBoxParam(fceu_hInstance, "7ZIPARCHIVEDIALOG", hParent, ArchiveFileSelectorCallback, (LPARAM)0);
				if(ret != LB_ERR)
				{
					FileSelectorContext::Item& item = fileSelectorContext.items[ret];
					std::vector<char> data(item.size);
					OutStream outStream( item.index, &data[0], item.size);
					const uint32 indices[1] = {item.index};
					if (SUCCEEDED(object->Extract(indices,1,0,&outStream)))
					{
						char* tempname = tmpnam(0);
						FILE* outf = fopen(tempname,"wb");
						fwrite(&data[0],1,item.size,outf);
						fclose(outf);
						void ALoad(char *nameo,char* actualfile);
						ALoad((char*)item.name.c_str(),tempname);

					} //if we extracted the file correctly

				} //if returned a file from the fileselector
				
			} //if we scanned the 7z correctly
		} //if we opened the 7z correctly
		object->Release();
	}
}