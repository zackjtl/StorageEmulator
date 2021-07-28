#include "stor_emulator.h"
#include <string>
#include <map>

using namespace std;

map<string, FILE*> _drive_file_map;
map<int, string> _handle_drive_map;

typedef struct {
  unsigned int PhysicalAddr;
  unsigned int Offset;
  unsigned int Count;
  unsigned int Reserved;
} TBlockDescriptor;

inline bool file_exists(const string& name)
{
  if (FILE* file = fopen(name.c_str(), "r")) {
    fclose(file);
    return true;
  }
  return false;
}

// Open drive file
int Open(const char* DriveName)
{
  map<string, FILE*>::iterator it = _drive_file_map.find(DriveName);

  if (it != _drive_file_map.end()) {
    // When the FILE exists in the drive map, it has been opened so that can't open it again
    return -1;
  }
  string driveFileName = DriveName + (string)".drv";

  FILE* file = NULL;
  if (!file_exists(DriveName)) {
    file = fopen(driveFileName.c_str(), "w+");
  }
  else {
    file = fopen(driveFileName.c_str(), "r+");
  }
  if (file == NULL)
    return -1;

  _drive_file_map.insert(make_pair(DriveName, file));

  int index = rand() % 4096;// distance(_drive_file_map.begin(), _drive_file_map.find(DriveName));

  _handle_drive_map.insert(make_pair(index, DriveName));

  return index;
}

// Close drive file
void Close(int Handle)
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
void InsertData(FILE* File, int Offset, int Length, byte* Buffer)
{
  int fSize = GetFileSize(File);
  fseek(File, Offset, SEEK_SET);

  byte* temp = new byte[fSize - Offset];
  fread(temp, 1, fSize - Offset, File);

  fseek(File, Offset + Length, SEEK_SET);
  fwrite(temp, 1, fSize - Offset, File);
  fseek(File, Offset, SEEK_SET);
  fwrite(Buffer, 1, Length, File);

  delete[] temp;
  return;
}

// Write Sector
int WriteSector(int Handle, int Sector, int Count, byte* Buffer)
{
  int end = Sector + Count;

  map<int, string>::iterator sit = _handle_drive_map.find(Handle);

  if (sit == _handle_drive_map.end())
    return;
  map<string, FILE*>::iterator fit = _drive_file_map.find(sit->second);

  int fSize = GetFileSize(fit->second);

  if (fSize < 4) 
  {
    // todo: Write data to a new file

    return Count;
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

  for (int sec = Sector; sec < end; ++sec)
  {
    if (sec >= mapCount) 
    {
      // todo: exceed map count
    }
    else {   
      if (SectorExists(map, sec))
      {
        int order = GetSectorRealOrder(map, sec);

        if (fSize < (dataStartOffset + order * 512 + 512)) {
          return -1;
        }
        fseek(fit->second, dataStartOffset + order * 512, SEEK_SET);
        fwrite(source, 1, 512, fit->second);
      }
      else {
        // todo: insert data
      }
    }



  }
  delete[] map;
}