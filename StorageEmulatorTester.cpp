#include <vcl.h>
#include <windows.h>

#pragma hdrstop
#pragma argsused

#include <tchar.h>
#include <stdio.h>
#include <iostream.h>
#include "store_emulator.h"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	int totalSectors = 32 * 1024 * 1024 * 2 - 32;
	int written = 0;
	INT readed = 0;
	byte* bufferA = new byte[65536];
	byte* bufferB = new byte[65536];
	byte* bufferC = new byte[65536];

	bool ret = DriveCreate("MyDrive", totalSectors);

	int handle = DriveOpen("MyDrive");

	if (handle == -1) {
		cout << "Open Handle Failed" << endl;
		goto END;
	}
	memset(bufferA, DriveEmptyData, 65536);
	memset(bufferB, DriveEmptyData, 65536);
	memset(bufferC, DriveEmptyData, 65536);

	for (int i = 0; i < 20; ++i) {
		memset(&bufferC[i * 512], i & 0xff, 512);
	}
	for (int i = 30; i < 32; ++i) {
		memset(&bufferC[i * 512], i & 0xff, 512);
	}
	for (int i = 50; i < 52; ++i) {
		memset(&bufferC[i * 512], i & 0xff, 512);
	}
	// Write New Test (Sector 4 ~ 9)
	written = DriveWriteSector(handle, 4, 6, &bufferC[4*512]);

	if (written != 6 * 512) {
		cout << "Write New Data Error" << endl;
	}

	memset(&bufferC[4 * 512], 0xFD, 512);
	memset(&bufferC[5 * 512], 0xFE, 512);

	// Insert & Overlay (rear) Test (Sector 0 ~ 5)
	written = DriveWriteSector(handle, 0, 6, &bufferC[0]);

	if (written != 6 * 512) {
		cout << "Write Overlay Rear Error" << endl;
	}
	memset(&bufferC[8 * 512], 0x33, 512);
	memset(&bufferC[9 * 512], 0x44, 512);

	// Write Overlay (tail) Test (Sector 8 ~ 13)
	written = DriveWriteSector(handle, 8, 6, &bufferC[8*512]);

	if (written != 6 * 512) {
		cout << "Write Overlay Tail Error" << endl;
		goto END;
	}

	// Write Append Test (14 ~ 19)
	written = DriveWriteSector(handle, 14, 6, &bufferC[14*512]);

	if (written != 6 * 512) {
		cout << "Write Append Error" << endl;
	}

	// Jump Append Test (50 ~ 51)
	written = DriveWriteSector(handle, 50, 2, &bufferC[50*512]);

	if (written != 2 * 512) {
		cout << "Jump Append Error" << endl;
	}

	// Jump Insert Test (30 ~ 31)
	written = DriveWriteSector(handle, 30, 2, &bufferC[30*512]);

	if (written != 2 * 512) {
		cout << "Jump Append Error" << endl;
	}

	readed = DriveReadSector(handle, 0, 60, &bufferB[0*512]);

	if (memcmp(bufferC, bufferB, 65536) != 0) {
		FILE* fileC = fopen("bufferC.bin", "wb+");
		FILE* fileB = fopen("bufferB.bin", "wb+");
		fwrite(bufferC, 1, 65536, fileC);
		fwrite(bufferB, 1, 65536, fileB);
		fclose(fileC);
		fclose(fileB);

  	cout << "Pattern Verification Fail" << endl;
	}

TEST_DONE:

END:
	DriveClose(handle);
	delete[] bufferA, bufferB, bufferC;
	return 0;
}
