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

int filesOpened = 0;

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
  heap_header->block_count = 1;

  //Close file
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  

  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(handler));


  return 0;
}

int HeapFile_Open(const char *fileName, int *file_handle, HeapFileHeader** header_info){

  if (filesOpened++ > BF_MAX_OPEN_FILES) {
    return BF_OPEN_FILES_LIMIT_ERROR;
  }

  CALL_BF(BF_OpenFile(fileName, file_handle));
  
  BF_Block *block;
  BF_Block_Init(&block);
  
  void *data;
  HeapFileHeader* dummy;

  CALL_BF(BF_GetBlock(*file_handle, 0, block));
  data = block;
  dummy = data;
  *header_info = malloc(sizeof(HeapFileHeader*));
  (*header_info)->block_count = dummy->block_count;
  BF_Block_Destroy(&block);
  
  return 0;
}

int HeapFile_Close(int file_handle, HeapFileHeader *hp_info){

  filesOpened--;

  BF_Block *header_block;
  BF_Block_Init(&header_block);
  HeapFileHeader* dummy;

  CALL_BF(BF_GetBlock(file_handle, 0, header_block));

  
  void* data = BF_Block_GetData(header_block);
  dummy = data;
  dummy->block_count = hp_info->block_count;

  BF_Block_SetDirty(header_block);
  CALL_BF(BF_UnpinBlock(header_block));
  
  
  // ? Maybe issue here
  BF_Block_Destroy(&header_block);
  
  CALL_BF(BF_CloseFile(file_handle));
  free(hp_info);
  return 0;
}

int HeapFile_InsertRecord(int file_handle, HeapFileHeader *hp_info, const Record record){
  
  if (hp_info->block_count >= BF_BUFFER_SIZE) {
    return BF_FULL_MEMORY_ERROR;
  }

  BF_Block* Block;
  BF_Block_Init(&Block);

  
  CALL_BF(BF_GetBlock(file_handle, hp_info->block_count, Block));
  void* data = BF_Block_GetData(Block);

  if (sizeof(record) > BF_BLOCK_SIZE - sizeof(data)) {

    CALL_BF(BF_AllocateBlock(file_handle, Block));
    data = BF_Block_GetData(Block);

    const Record *rec = data;
    rec = &record;

    BF_Block_SetDirty(Block);
    CALL_BF(BF_UnpinBlock(Block));

    hp_info->block_count++;

  } else {

    const Record *rec = data;
    rec = &record;

    BF_Block_SetDirty(Block);
    CALL_BF(BF_UnpinBlock(Block));
  }

  BF_Block_Destroy(&Block);

  return 0;
}


HeapFileIterator HeapFile_CreateIterator(int file_handle, HeapFileHeader* header_info, int id)  {

  HeapFileIterator out;
  out.idToSearch = id;

  BF_Block* blockIterate;
  BF_Block_Init(&blockIterate);
  void* data;
  int foundId = 0;
  Record* rec;
  for (int i = 1; i < BF_BUFFER_SIZE; i ++) {
    BF_GetBlock(file_handle, i, blockIterate);
    data = BF_Block_GetData(blockIterate);
    rec = data;

    for (int j = 0; j < sizeof(rec)/sizeof(rec[0]); j++) {
      if (rec[j].id == id) {
        foundId = 1;
        break;
      }
    }

    if (foundId) {
      out.blockIndex = i;
      break;
    }
  }

  BF_Block_Destroy(&blockIterate);

  return out;
}


int HeapFile_GetNextRecord(HeapFileIterator* heap_iterator, Record** record)  {

    * record=NULL;
    return 1;
}

