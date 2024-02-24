#pragma once

#include <Windows.h>
#include <stdint.h>
#include <string>

// High-performance class for writing a series of text lines to a file, using overlapped, unbuffered I/O
// Works on Windows builds both SDL/Qt and non-SQL/Qt
// Apart from getLastError, the entire API can be adapted to other OS with no client changes
class TraceFileWriter
{
public:
	static const size_t BlockSize = 4 << 10;
	static const size_t BuffSize = BlockSize * 257;
	static const size_t FlushSize = BlockSize * 256;

	inline TraceFileWriter()
	{
		// Windows Vista API that allows setting the end of file without closing and reopening it
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

	// SUPPOSED to always be valid (ERROR_SUCCESS if no error), but bugs may make it only valid after a failure
	inline DWORD getLastError() const
	{
		return lastErr;
	}

	// Open the file and allocate all necessary resources
	bool open(const char *fileName, bool isPaused = false)
	{
		HANDLE file = CreateFileA(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			lastErr = GetLastError();
			return false;
		}

		initialize(false, isPaused, fileName, file);

		// Allocate resources
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
			// Free resources on failure
			cleanup();

		return isOpen;
	}

	// Close the file and free resources
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

	// When going from unpaused to paused, flush file and set end of file so it can be accessed externally
	bool setPause(bool isPaused)
	{
		if (isPaused && !this->isPaused)
		{
			// Wait for any outstanding writes to complete
			for (unsigned i = 0; i < 2; i++)
			{
				if (!waitForBuffer(i))
					return false;
			}

			// Write out anything still in the buffer
			if (!writeTail())
				return false;
		}

		this->isPaused = isPaused;
		lastErr = ERROR_SUCCESS;

		return true;
	}

	// Add a line to the buffer and write it out when the buffer is filled
	// Under most failure cirumstances the line is added to the buffer
	bool writeLine(const char *line, bool addEol = true)
	{
		if (!isOpen)
		{
			lastErr = ERROR_FILE_NOT_FOUND;
			return false;
		}

		// Add to buffer
		static const char eol[] = "\r\n";
		size_t eolSize = strlen(eol);
		char *buff = buffers[buffIdx];
		size_t lineLen = strlen(line);
		size_t copyLen = lineLen + (addEol ? eolSize : 0);
		if (buffOffs + lineLen + eolSize > BuffSize)
		{
			// Buffer is full. This shouldn't ever happen.
			lastErr = ERROR_INTERNAL_ERROR;
			return false;
		}

		memcpy(buff + buffOffs, line, lineLen);
		if (addEol)
			memcpy(buff + buffOffs + lineLen, eol, eolSize);
		buffOffs += lineLen + eolSize;

		// Check if the previous write is done, to detect it as early as possible
		unsigned prevBuff = (buffIdx + 1) % 2;
		if (!waitForBuffer(prevBuff, 0) && lastErr != ERROR_TIMEOUT)
			return false;

		lastErr = ERROR_SUCCESS;

		if (buffOffs < FlushSize)
			return true;

		return writeBlocks();
	}

	// Flush buffer contents. Writes partial blocks, but does NOT set end of file
	// Do NOT call frequently, as writes may be significantly undersized and cause poor performance
	bool flush()
	{
		if (!isOpen)
		{
			lastErr = ERROR_FILE_NOT_FOUND;
			return false;
		}

		// Write full blocks, if any
		if (!writeBlocks())
			return false;

		char *buff = buffers[buffIdx];
		if (buffOffs != 0)
		{
			// Write out partial block at the tail
			size_t writeSize = (buffOffs + BlockSize - 1) & ~(BlockSize - 1);
			memset(buff + buffOffs, ' ', writeSize - buffOffs);

			if (!beginWrite(writeSize)
				|| !waitForBuffer(buffIdx))
				return false;

			// Do NOT update buffIdx, buffOffs, or fileOffs, as the partial block must be overwritten later
		}

		// Wait for all writes to complete
		for (unsigned i = 0; i < 2; i++)
		{
			if (!waitForBuffer(i))
				return false;
		}

		lastErr = ERROR_SUCCESS;
		return true;
	}

protected:
	typedef BOOL (WINAPI *SetFileInformationByHandlePtr)(
		HANDLE                    hFile,
		FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
		LPVOID                    lpFileInformation,
		DWORD                     dwBufferSize
	);

	SetFileInformationByHandlePtr SetFileInformationByHandle;

	const size_t NO_WRITE = size_t(-1);

	DWORD lastErr;

	bool isOpen;
	bool isPaused;

	std::string fileName;
	HANDLE file;

	// Double-buffers
	char *buffers[2];
	OVERLAPPED ovls[2];
	size_t bytesToWrite[2]; // Write in progress size or size_t(-1) if none

	unsigned buffIdx;
	size_t buffOffs;
	uint64_t fileOffs;

	// Put the class into a defined state, but does NOT allocate resources
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
			bytesToWrite[i] = NO_WRITE;
		}

		memset(ovls, 0, sizeof(ovls));

		buffIdx = 0;
		buffOffs = 0;
		fileOffs = 0;
	}

	// Close file and release resources. Does NOT wait for writes to complete.
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

	// Write out as many blocks as present in the buffer
	bool writeBlocks()
	{
		if (buffOffs < BlockSize)
			return true;

		char *buff = buffers[buffIdx];
		unsigned prevBuff = (buffIdx + 1) % 2;
		OVERLAPPED *ovl = &ovls[buffIdx];

		DWORD writeSize = buffOffs - (buffOffs % BlockSize);
		if (!beginWrite(writeSize))
			return false;

		bool isAsync = bytesToWrite[buffIdx] != NO_WRITE;
		if (isAsync)
		{
			if (!waitForBuffer(prevBuff))
				return false; // Catastrophic failure case

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

		bytesToWrite[buffIdx] = NO_WRITE;

		return true;
	}

	// Begin a write and handle errors without updating class state
	bool beginWrite(size_t writeSize)
	{
		bytesToWrite[buffIdx] = NO_WRITE;

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

	// Set the end of file so it can be accessed
	bool setEndOfFile()
	{
		if (SetFileInformationByHandle != nullptr)
		{
			// Easy case: Vista or better
			FILE_END_OF_FILE_INFO eofInfo;
			eofInfo.EndOfFile.QuadPart = int64_t(fileOffs + buffOffs);

			if (!SetFileInformationByHandle(file, FileEndOfFileInfo, &eofInfo, sizeof(eofInfo)))
			{
				lastErr = GetLastError();
				return false;
			}

			lastErr = ERROR_SUCCESS;
			return true;
		}
		else
		{
			// Hard case: XP
			// If set EOF fails, make a desperate attempt to reopen the file and keep running
			lastErr = ERROR_SUCCESS;
			do
			{
				// Set EOF to fileOffs rounded up to the next block
				LARGE_INTEGER tgtOffs;
				tgtOffs.QuadPart = (fileOffs + buffOffs + BlockSize - 1) & ~(BlockSize - 1);
				LARGE_INTEGER newOffs;

				if (!SetFilePointerEx(file, tgtOffs, &newOffs, FILE_BEGIN)
					|| !SetEndOfFile(file))
					break;
				
				CloseHandle(file);

				// Open file with buffering so the exact byte size can be set
				tgtOffs.QuadPart = fileOffs + buffOffs;

				file = CreateFileA(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
				if (file != INVALID_HANDLE_VALUE)
				{
					if (!SetFilePointerEx(file, tgtOffs, &newOffs, FILE_BEGIN)
						|| !SetEndOfFile(file))
						lastErr = GetLastError();

					CloseHandle(file);
				}
				else
					lastErr = GetLastError();

				// Finally, reopen the file in original mode
				file = CreateFileA(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
					FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, nullptr);
				if (file == INVALID_HANDLE_VALUE || lastErr)
					break;

				lastErr = ERROR_SUCCESS;
				return true;
			} while (false);

			// Failed
			if (!lastErr)
				lastErr = GetLastError();

			return false;
		}
	}

	// Write out everything in the buffer and set the file end for pausing
	inline bool writeTail()
	{
		return flush() && setEndOfFile();
	}

	// Wait for an a buffer to become available, waiting for write completion if necessary
	bool waitForBuffer(unsigned buffIdx, DWORD timeout = INFINITE)
	{
		if (buffIdx >= 2)
			lastErr = ERROR_INTERNAL_ERROR;
		else if (bytesToWrite[buffIdx] == NO_WRITE)
			// No write in progress
			lastErr = ERROR_SUCCESS;
		else
		{
			// Wait for the operation to complete
			DWORD waitRes = WaitForSingleObject(ovls[buffIdx].hEvent, timeout);
			if (waitRes == WAIT_TIMEOUT)
				lastErr = ERROR_TIMEOUT;
			else if (waitRes != WAIT_OBJECT_0)
			{
				lastErr = GetLastError();
				bytesToWrite[buffIdx] = NO_WRITE;
			}
			else
			{
				// Verify it succeeded
				DWORD prevBytesWritten;
				if (!GetOverlappedResult(file, &ovls[buffIdx], &prevBytesWritten, FALSE))
					lastErr = GetLastError();
				else if (prevBytesWritten != bytesToWrite[buffIdx])
					lastErr = ERROR_WRITE_FAULT;
				else
					lastErr = ERROR_SUCCESS;

				bytesToWrite[buffIdx] = NO_WRITE;
			}
		}

		return !lastErr;
	}
};
