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

// TODO: Find why the block number doesn't exist in the file.
// ! Huge leaks at CreateIterator

// Count for the open files
int filesOpened = 0;

int HeapFile_Create(const char* fileName){

  CALL_BF(BF_CreateFile(fileName));

  //Create Header Block
  int handler;
  BF_Block *block;
  void *data;
  
  // Initialize block
  BF_Block_Init(&block);

  // Open file and allocate a new block
  CALL_BF(BF_OpenFile(fileName, &handler));
  CALL_BF(BF_AllocateBlock(handler, block));

  data = BF_Block_GetData(block); // Get the data of the new block

  // Assigning the data into the header
  HeapFileHeader *heap_header = data;
  heap_header->block_count = 1;

  //Dirty, unpin and destroy the block, then close file
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(handler));


  return 1;
}

int HeapFile_Open(const char *fileName, int *file_handle, HeapFileHeader** header_info){

  // If the opened files exceed the maximum allowed then stop
  if (filesOpened++ > BF_MAX_OPEN_FILES) {
    return 0;
  }

  // Open the BF file and then initialize a block
  CALL_BF(BF_OpenFile(fileName, file_handle));
  BF_Block *block;
  BF_Block_Init(&block);
  
  // Create a pointer to manipulate the data of the block and a dummy heap pointer
  void *data;
  HeapFileHeader* dummy;

  /** Get the block at index 0 "header block" and then assign it's content to the data variable
  *   Assign the data to the dummy, initialize through malloc the header_info double pointer
  *   And add to it all the attributes of the dummy header and then destroy the node
  */
  CALL_BF(BF_GetBlock(*file_handle, 0, block));
  data = block;
  dummy = data;
  *header_info = malloc(sizeof(HeapFileHeader));
  (*header_info)->block_count = dummy->block_count;
  BF_Block_Destroy(&block);
  
  return 1;
}

int HeapFile_Close(int file_handle, HeapFileHeader *hp_info){

  // If there is no open file to close return 0
  if (filesOpened == 0)
    return 0;

  // Subtracting the amount of files opened
  filesOpened--;

  // Initializing a block and another dummy header
  BF_Block *header_block;
  BF_Block_Init(&header_block);
  HeapFileHeader* dummy;

  // Get the header block
  CALL_BF(BF_GetBlock(file_handle, 0, header_block));

  // Fill the dummy header with the data from the first block and the attributes of the actual header
  void* data = BF_Block_GetData(header_block);
  dummy = data;
  dummy->block_count = hp_info->block_count;

  // Dirty and unpin the header block
  BF_Block_SetDirty(header_block);
  CALL_BF(BF_UnpinBlock(header_block));
  
  
  // Destroy the block because we don't need it and the close the file and free the header to avoid leaks
  BF_Block_Destroy(&header_block);
  CALL_BF(BF_CloseFile(file_handle));
  free(hp_info);

  return 1;
}

int HeapFile_InsertRecord(int file_handle, HeapFileHeader *hp_info, const Record record){
  
  if (hp_info->block_count >= BF_BUFFER_SIZE) {
    return BF_FULL_MEMORY_ERROR;
  }

  BF_Block* Block;
  BF_Block_Init(&Block);

  printf("%d\n", hp_info->block_count++);
  CALL_BF(BF_GetBlock(file_handle, hp_info->block_count, Block));
  void* data = BF_Block_GetData(Block);

  if (sizeof(record) > BF_BLOCK_SIZE - sizeof(data) || hp_info->block_count) {

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

  return 1;
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

