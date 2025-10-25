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
  
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(handler));


  return 0;
}

int HeapFile_Open(const char *fileName, int *file_handle, HeapFileHeader** header_info){

  CALL_BF(BF_OpenFile(fileName, file_handle));
  
  BF_Block *block;
  BF_Block_Init(&block);
  
  void *data;

  CALL_BF(BF_GetBlock(*file_handle, 0, block));
  data = block;
  *header_info = data;

  return 0;
}

// TODO: Check if the header needs to be re-inserted
int HeapFile_Close(int file_handle, HeapFileHeader *hp_info){

  void *header = hp_info;
  BF_Block *header_block = header;

  BF_Block_SetDirty(header_block);
  CALL_BF(BF_UnpinBlock(header_block));

  // ? Maybe issue here
  BF_Block_Destroy(&header_block);
  CALL_BF(BF_CloseFile(file_handle));

  return 0;
}

/** 
* TODO: Find a way (if needed) to check if the block is empty or not
* ? δημιουργώντας αυτόματα ένα νέο μπλοκ *εάν το τρέχον είναι πλήρες* (Εργασία1.pdf)
**/
int HeapFile_InsertRecord(int file_handle, HeapFileHeader *hp_info, const Record record){
    
  void* header = hp_info;
  BF_Block* Block = header;

  void* data = BF_Block_GetData(Block);
  if (data != NULL) {
    CALL_BF(BF_AllocateBlock(file_handle, Block));
    data = BF_Block_GetData(Block);
    const Record *rec = data;

    rec = &record;

    BF_Block_SetDirty(Block);
    CALL_BF(BF_UnpinBlock(Block));

    header = Block;
    hp_info = header;
  } else {
    const Record *rec = data;

    rec = &record;

    BF_Block_SetDirty(Block);
    CALL_BF(BF_UnpinBlock(Block));
  }
  return 0;
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

