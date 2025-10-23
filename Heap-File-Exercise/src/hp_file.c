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

int HeapFile_Create(const char* fileName){

  CALL_BF(BF_CreateFile(fileName));

  //Create Header Block
  int handler;
  BF_Block *block;
  void *data;
  
  BF_Block_Init(&block);

  CALL_BF(BF_OpenFile(fileName, &handler));
  CALL_BF(BF_AllocateBlock(handler, block));

  data = BF_Block_GetData(block);
  HeapFileHeader *heap_header = data;
  heap_header->block_count = 0;
  heap_header->first = NULL;
  heap_header->last = NULL;

  //Close file
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  CALL_BF(BF_CloseFile(handler));

  



  return 0;
}

int HeapFile_Open(const char *fileName, int *file_handle, HeapFileHeader** header_info)
{
  CALL_BF(BF_OpenFile(fileName, file_handle));
  
  BF_Block *block;
  BF_Block_Init(&block);
  
  void *data;

  CALL_BF(BF_GetBlock(*file_handle, 0, block));
  data = block;
  *header_info = data;

  return 0;
}

// Remember to unpin the header block
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

