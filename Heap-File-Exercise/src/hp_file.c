#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file_structs.h"
#include "record.h"

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return 0;        \
    }                         \
  }

int HeapFile_Create(const char* fileName)
{
  CALL_BF(BF_CreateFile(fileName))
  return 0;
}

int HeapFile_Open(const char *fileName, int *file_handle, HeapFileHeader** header_info)
{
  CALL_BF(BF_OpenFile(fileName, file_handle));
  
  int count;
  CALL_BF(BF_GetBlockCounter(*file_handle, &count));
  if (count > 0)
  {
    BF_Block *block;
    CALL_BF(BF_BlockInit(&block))
    CALL_BF(BF_GetBlock(*file_handle, 0, block));
  }
  else
  {

  }
}

// Remember to unpin the block
int HeapFile_Close(int file_handle, HeapFileHeader *hp_info)
{
  return 1;
}

int HeapFile_InsertRecord(int file_handle, HeapFileHeader *hp_info, const Record record)
{
  return 1;
}


HeapFileIterator HeapFile_CreateIterator(    int file_handle, HeapFileHeader* header_info, int id)
{
  HeapFileIterator out;
  return out;
}


int HeapFile_GetNextRecord(    HeapFileIterator* heap_iterator, Record** record)
{
    * record=NULL;
    return 1;
}

