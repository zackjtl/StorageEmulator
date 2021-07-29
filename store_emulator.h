#ifndef STORE_EMULATORH
#define STORE_EMULATORH

extern unsigned char DriveEmptyData;

bool DriveCreate(const char* DriveName, int TotalSectors);
int DriveOpen(const char* DriveName);
void DriveClose(int Handle);
int DriveWriteSector(int Handle, int Sector, int Count, byte* Buffer);
int DriveReadSector(int Handle, int Sector, int Count, byte* Buffer);

#endif
