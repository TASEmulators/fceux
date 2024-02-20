#pragma once

#include <Windows.h>
#include <stdint.h>
#include <string>

class TraceFileWriter
{
public:
	static const size_t BlockSize = 4 << 10;
	static const size_t BuffSize = BlockSize * 257;
	static const size_t FlushSize = BlockSize * 256;

	inline TraceFileWriter()
	{
		HMODULE kernel32 = GetModuleHandleA("kernel32");
		SetFileInformationByHandle = kernel32 ? (SetFileInformationByHandlePtr)GetProcAddress(kernel32, "SetFileInformationByHandle") : nullptr;

		initialize();
	}

	inline ~TraceFileWriter()
	{
		if (isOpen)
			close();
	}

	inline bool getOpen() const
	{
		return isOpen;
	}

	inline DWORD getLastError() const
	{
		return lastErr;
	}

	bool open(const char *fileName, bool isPaused = false)
	{
		HANDLE file = CreateFileA(fileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);
		if (file == INVALID_HANDLE_VALUE)
		{
			lastErr = GetLastError();
			return false;
		}

		initialize(false, isPaused, fileName, file);

		do
		{
			for (unsigned i = 0; i < 2; i++)
			{
				buffers[i] = (char *)VirtualAlloc(NULL, BuffSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
				if (buffers[i] == nullptr)
				{
					lastErr = GetLastError();
					break;
				}

				ovls[i].hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
				if (ovls[i].hEvent == nullptr)
				{
					lastErr = GetLastError();
					break;
				}
			}

			if (!lastErr)
				isOpen = true;
		} while (false);

		if (!isOpen)
			cleanup();

		return isOpen;
	}

	void close()
	{
		if (!isOpen)
		{
			lastErr = ERROR_FILE_NOT_FOUND;
			return;
		}

		for (unsigned i = 0; i < 2; i++)
			waitForBuffer(i);

		writeTail();

		cleanup();
	}

	bool setPause(bool isPaused)
	{
		if (isPaused && !this->isPaused)
		{
			for (unsigned i = 0; i < 2; i++)
			{
				if (!waitForBuffer(i))
					return false;
			}

			if (!writeTail())
				return false;
		}

		this->isPaused = isPaused;
		lastErr = ERROR_SUCCESS;

		return true;
	}

	bool writeLine(const char *line)
	{
		if (!isOpen)
		{
			lastErr = ERROR_FILE_NOT_FOUND;
			return false;
		}

		char *buff = buffers[buffIdx];
		OVERLAPPED *ovl = &ovls[buffIdx];

		size_t lineLen = strlen(line);
		if (buffOffs + lineLen + 1 > BuffSize)
		{
			lastErr = ERROR_INTERNAL_ERROR;
			return false;
		}

		memcpy(buff + buffOffs, line, lineLen);
		buffOffs += lineLen;
		buff[buffOffs++] = '\n';

		unsigned prevBuff = (buffIdx + 1) % 2;
		if (!waitForBuffer(prevBuff, 0) && lastErr != ERROR_TIMEOUT)
			return false;

		lastErr = ERROR_SUCCESS;

		if (buffOffs < FlushSize)
			return true;

		DWORD writeSize = buffOffs - (buffOffs % BlockSize);
		if (!beginWrite(writeSize))
			return false;

		bool isAsync = bytesToWrite[buffIdx] != INVALID_FILE_SIZE;
		if (isAsync)
		{
			if (!waitForBuffer(prevBuff))
				return false;

			// Switch to next buffer

			memcpy(buffers[prevBuff], buff + writeSize, buffOffs - writeSize);
			buffOffs -= writeSize;

			fileOffs += writeSize;

			buffIdx = prevBuff;
		}
		else
		{
			// Stick with same buffer
			memmove(buff, buff + writeSize, buffOffs - writeSize);

			buffOffs -= writeSize;
		}

		bytesToWrite[buffIdx] = INVALID_FILE_SIZE;

		return true;
	}

protected:
	typedef BOOL (*SetFileInformationByHandlePtr)(
		HANDLE                    hFile,
		FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
		LPVOID                    lpFileInformation,
		DWORD                     dwBufferSize
	);

	SetFileInformationByHandlePtr SetFileInformationByHandle;

	DWORD lastErr;

	bool isOpen;
	bool isPaused;

	std::string fileName;
	HANDLE file;

	char *buffers[2];
	OVERLAPPED ovls[2];
	size_t bytesToWrite[2];

	unsigned buffIdx;
	size_t buffOffs;
	uint64_t fileOffs;

	void initialize(bool isOpen = false, bool isPaused = false, const char *fileName = "", HANDLE file = INVALID_HANDLE_VALUE)
	{
		lastErr = ERROR_SUCCESS;

		this->isOpen = isOpen;
		this->isPaused = isPaused;

		this->fileName = fileName;
		this->file = file;

		for (unsigned i = 0; i < 2; i++)
		{
			buffers[i] = nullptr;
			bytesToWrite[i] = INVALID_FILE_SIZE;
		}

		memset(ovls, 0, sizeof(ovls));

		buffIdx = 0;
		buffOffs = 0;
		fileOffs = 0;
	}

	void cleanup()
	{
		isOpen = false;

		if (file != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file);
			file = INVALID_HANDLE_VALUE;
		}

		for (unsigned i = 0; i < 2; i++)
		{
			if (buffers[i] != nullptr)
				VirtualFree(buffers[i], 0, MEM_RELEASE);
			if (ovls[i].hEvent != nullptr)
				CloseHandle(ovls[i].hEvent);

			buffers[i] = nullptr;
			ovls[i].hEvent = nullptr;
		}
	}

	bool beginWrite(size_t writeSize)
	{
		bytesToWrite[buffIdx] = INVALID_FILE_SIZE;

		if (writeSize % BlockSize != 0)
		{
			lastErr = ERROR_INTERNAL_ERROR;
			return false;
		}

		OVERLAPPED *ovl = &ovls[buffIdx];
		ovl->Offset = (DWORD)fileOffs;
		ovl->OffsetHigh = DWORD(fileOffs >> 32);

		bool success = false;
		DWORD bytesWritten;
		if (WriteFile(file, buffers[buffIdx], writeSize, &bytesWritten, ovl))
		{
			// Completed synchronously
			lastErr = (bytesWritten == writeSize)
				? ERROR_SUCCESS
				: ERROR_WRITE_FAULT; // "Close enough"
		}
		else
		{
			DWORD lastErr = GetLastError();
			if (lastErr == ERROR_IO_PENDING)
			{
				bytesToWrite[buffIdx] = writeSize;
				lastErr = ERROR_SUCCESS;
			}
		}

		return !lastErr;
	}

	bool writeTail()
	{
		char *buff = buffers[buffIdx];
		if (buffOffs != 0)
		{
			size_t writeSize = (buffOffs + BlockSize - 1) & ~(BlockSize - 1);
			memset(buff + buffOffs, ' ', writeSize - buffOffs);

			if (!beginWrite(writeSize)
				|| !waitForBuffer(buffIdx))
				return false;
		}

		if (SetFileInformationByHandle != nullptr)
		{
			FILE_END_OF_FILE_INFO eofInfo;
			eofInfo.EndOfFile.QuadPart = int64_t(fileOffs + buffOffs);

			if (!SetFileInformationByHandle(file, FileEndOfFileInfo, &eofInfo, sizeof(eofInfo)))
			{
				lastErr = GetLastError();
				return false;
			}
		}

		return true;
	}

	bool waitForBuffer(unsigned buffIdx, DWORD timeout = INFINITE)
	{
		if (buffIdx >= 2)
			lastErr = ERROR_INTERNAL_ERROR;
		else if (bytesToWrite[buffIdx] == INVALID_FILE_SIZE)
			lastErr = ERROR_SUCCESS;
		else
		{
			DWORD waitRes = WaitForSingleObject(ovls[buffIdx].hEvent, timeout);
			if (waitRes == WAIT_TIMEOUT)
				lastErr = ERROR_TIMEOUT;
			else if (waitRes != WAIT_OBJECT_0)
			{
				lastErr = GetLastError();
				bytesToWrite[buffIdx] = INVALID_FILE_SIZE;
			}
			else
			{
				DWORD prevBytesWritten;
				if (!GetOverlappedResult(file, &ovls[buffIdx], &prevBytesWritten, FALSE))
					lastErr = GetLastError();
				else if (prevBytesWritten != bytesToWrite[buffIdx])
					lastErr = ERROR_WRITE_FAULT;
				else
					lastErr = ERROR_SUCCESS;

				bytesToWrite[buffIdx] = INVALID_FILE_SIZE;
			}
		}

		return !lastErr;
	}
};