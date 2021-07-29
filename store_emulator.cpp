#include "stor_emulator.h"
#include <string>
#include <map>

#if defined(__BCPLUSPLUS__)
#include <vcl.h>
#endif

using namespace std;

map<string, FILE*> _drive_file_map;
map<int, string> _handle_drive_map;

unsigned char DriveEmptyData = 0xFF;

inline bool file_exists(const string& name)
{
  if (FILE* file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  }
  return false;
}

// Create a new drive
bool DriveCreate(const char* DriveName, int TotalSectors)
{
  string driveFileName = DriveName + (string)".drv";

  if (file_exists(driveFileName)) {
    return false;
  }
  FILE* file = NULL;
  file = fopen(driveFileName.c_str(), "wb+");

  if (file == NULL)
    return false;

  int mapSize = (TotalSectors + 7) / 8;

  byte* headerBuff = new byte[mapSize + 4];
  int* startOffset = (int*)headerBuff;

  memset(headerBuff, 0, mapSize + 4);
  *startOffset = 4 + mapSize;

  fwrite(headerBuff, 1, 4 + mapSize, file);

  fclose(file);
  delete[] headerBuff;
}

// Open drive file
int DriveOpen(const char* DriveName)
{
  map<string, FILE*>::iterator it = _drive_file_map.find(DriveName);

  if (it != _drive_file_map.end()) {
    // When the FILE exists in the drive map, it has been opened so that can't open it again
    return -1;
  }
  string driveFileName = DriveName + (string)".drv";

  FILE* file = NULL;
  if (!file_exists(driveFileName)) {
    return -1;
  }
  else {
    // Existing drive
    file = fopen(driveFileName.c_str(), "rb+");
  }
  if (file == NULL)
    return -1;

  _drive_file_map.insert(make_pair(DriveName, file));

  int index = rand() % 4096;// distance(_drive_file_map.begin(), _drive_file_map.find(DriveName));

  _handle_drive_map.insert(make_pair(index, DriveName));

  return index;
}

// Close drive file
void DriveClose(int Handle)
{
  map<int, string>::iterator sit = _handle_drive_map.find(Handle);

  if (sit == _handle_drive_map.end())
    return;
  map<string, FILE*>::iterator fit = _drive_file_map.find(sit->second);

  fclose(fit->second);

  _drive_file_map.erase(fit);
  _handle_drive_map.erase(sit);
}

#define SectorExists(Map, Sector) ((Map[Sector / 8] & (byte)(0x01 << (Sector & 7))) == (byte)0 ? false : true)

// Get physical order inside the file of the sector
inline int GetSectorRealOrder(byte* Map, int Sector)
{
  int order = 0;

  for (int i = 0; i < Sector; ++i)
  {
    if (SectorExists(Map, i)) {
      ++order;
    }
  }
  return order;
}

// Set Sector Existing Flag
inline void SetSectorFlag(byte* Map, int Sector, bool Exist)
{
  if (Exist)
    Map[Sector / 8] |= (byte)(0x01 << (Sector & 7));
  else
    Map[Sector / 8] &= ~((byte)(0x01 << (Sector & 7)));
}

// Get File Size
inline int GetFileSize(FILE* File)
{
  fseek(File, 0, SEEK_END);
  int fSize = ftell(File);
  fseek(File, 0, SEEK_SET);
  return fSize;
}

// Insert amount of data into a file
int InsertData(FILE* File, int Offset, int Length, byte* Buffer)
{
	int fSize = GetFileSize(File);
	fseek(File, Offset, SEEK_SET);

	if (fSize > Offset) {
		byte* temp = new byte[fSize - Offset];
		fread(temp, 1, fSize - Offset, File);

		fseek(File, Offset + Length, SEEK_SET);
		fwrite(temp, 1, fSize - Offset, File);
		fseek(File, Offset, SEEK_SET);
		delete[] temp;
	}
  int written = fwrite(Buffer, 1, Length, File);

  return written;
}

// Write Sector
int DriveWriteSector(int Handle, int Sector, int Count, byte* Buffer)
{
	int end = Sector + Count;
	int written = 0;

	map<int, string>::iterator sit = _handle_drive_map.find(Handle);

	// The input handle not opened
	if (sit == _handle_drive_map.end())
		return 0;

	map<string, FILE*>::iterator fit = _drive_file_map.find(sit->second);

	int fSize = GetFileSize(fit->second);

	if (fSize < 4)
	{
		// Incorrect drive file
		return 0;
	}
  byte buff[4];
  int readed = fread(buff, 1, 4, fit->second);
  int dataStartOffset = *(int*)buff;
  
  if (fSize < (dataStartOffset)) {
    return -1;
  }
	byte* map = new byte[dataStartOffset-4];
  byte* source = Buffer;

  fseek(fit->second, 4, SEEK_SET);
  fread(map, 1, dataStartOffset-4, fit->second);

	int mapCount = (dataStartOffset - 4) * 8;
	bool mapChanged = false;

	for (int sec = Sector; sec < end; ++sec)
	{
		if (sec >= mapCount) {
			// exceed drive size
			break;
		}
		source = Buffer + (sec - Sector) * 512;
		int order = GetSectorRealOrder(map, sec);
		int realAddress = dataStartOffset + order * 512;

		if (SectorExists(map, sec))
		{
			// Replace data
			if (fSize < (realAddress + 512)) {
				// Data length is not match to the existing flag
				delete[] map;
				return -1;
			}
			fseek(fit->second, realAddress, SEEK_SET);
			written += fwrite(source, 1, 512, fit->second);
		}
		else {
			// todo: insert data
			written += InsertData(fit->second, realAddress, 512, source);
			SetSectorFlag(map, sec, true);
			mapChanged = true;
		}
	}
	if (mapChanged) {
		fseek(fit->second, 4, SEEK_SET);
		fwrite(map, 1, dataStartOffset - 4, fit->second);
	}

	delete[] map;
	return written;
}

// Write Sector
int DriveReadSector(int Handle, int Sector, int Count, byte* Buffer)
{
	int end = Sector + Count;
	int readed = 0;

	map<int, string>::iterator sit = _handle_drive_map.find(Handle);

	// The input handle not opened
	if (sit == _handle_drive_map.end())
		return 0;

	map<string, FILE*>::iterator fit = _drive_file_map.find(sit->second);

	int fSize = GetFileSize(fit->second);

	if (fSize < 4)
	{
		// Incorrect drive file
		return 0;
	}
 	byte buff[4];
  int mapReaded = fread(buff, 1, 4, fit->second);
  int dataStartOffset = *(int*)buff;

	byte* map = new byte[dataStartOffset-4];
	byte* dest = Buffer;

	fseek(fit->second, 4, SEEK_SET);
  fread(map, 1, dataStartOffset-4, fit->second);

	int mapCount = (dataStartOffset - 4) * 8;
	bool mapChanged = false;

	for (int sec = Sector; sec < end; ++sec) {
		if (sec >= mapCount) {
			// exceed drive size
			break;
		}
		dest = Buffer + (sec - Sector) * 512;
		int order = GetSectorRealOrder(map, sec);
		int realAddress = dataStartOffset + order * 512;

		if (SectorExists(map, sec))
		{
			// Replace data
			if (fSize < (realAddress + 512)) {
				// Data length is not match to the existing flag
				delete[] map;
				return -1;
			}
			fseek(fit->second, realAddress, SEEK_SET);
			readed += fread(dest, 1, 512, fit->second);
		}
		else {
			// todo: insert data
			memset(dest, DriveEmptyData, 512);
			readed += 512;
		}
	}
	delete[] map;
	return readed;
}