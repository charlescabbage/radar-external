#pragma once

#include <windows.h>

class ProcMem
{

public:

	DWORD FindDmaAddy(HANDLE hProcess, DWORD dwBaseAddress, DWORD dwOffsets[], int pLvl)
	{
		DWORD pntrAddr = dwBaseAddress;
		for (int i = 0; i < pLvl; i++)
		{
			pntrAddr = Read<DWORD>(hProcess, pntrAddr);
			pntrAddr += dwOffsets[i];
		}
		return pntrAddr;
	}

	template <class T>
	T Read(HANDLE hProcess, DWORD dwAddress)
	{
		T buffer;
		ReadProcessMemory(hProcess, (LPVOID)dwAddress, &buffer, sizeof(T), NULL);
		return buffer;
	}

	template <class T>
	T Read(HANDLE hProcess, DWORD dwAddress, DWORD dwOffsets[], int pLvl)
	{
		return Read<T>(hProcess, FindDmaAddy(hProcess, dwAddress, dwOffsets, pLvl));
	}

	template <class T>
	void Write(HANDLE hProcess, DWORD dwAddress, T Value)
	{
		WriteProcessMemory(hProcess, (LPVOID)dwAddress, &Value, sizeof(T), NULL);
	}

	template <class T>
	void Write(HANDLE hProcess, DWORD dwAddress, DWORD dwOffsets[], T Value, int pLvl)
	{
		Write<T>(hProcess, FindDmaAddy(hProcess, dwAddress, dwOffsets, pLvl), Value);
	}

};