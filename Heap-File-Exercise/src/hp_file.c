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
  heap_header->blockCount = 1;
  heap_header->recordCount = 0;
  heap_header->totalRecords = 0;

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
  data = BF_Block_GetData(block);
  dummy = data;
  *header_info = (HeapFileHeader*)malloc(sizeof(HeapFileHeader));
  (*header_info)->blockCount = dummy->blockCount;
  (*header_info)->recordCount = dummy->recordCount;
  (*header_info)->totalRecords = dummy->totalRecords;
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
  dummy->blockCount = hp_info->blockCount;
  dummy->recordCount = hp_info->recordCount;
  dummy->totalRecords = hp_info->totalRecords;

  // Dirty and unpin the header block
  BF_Block_SetDirty(header_block);
  CALL_BF(BF_UnpinBlock(header_block));
  
  
  // Destroy the block because we don't need it and then close the file and free the header to avoid leaks
  BF_Block_Destroy(&header_block);
  CALL_BF(BF_CloseFile(file_handle));
  free(hp_info);

  return 1;
}

int HeapFile_InsertRecord(int file_handle, HeapFileHeader *hp_info, const Record record){
  
  // Initializing a block
  BF_Block* block;
  BF_Block_Init(&block);

  /**
  * Getting the first block (blockCount) starts at 1 but GetBlock has it's first block at 0,
  * then extracting the data and increase the total records. If blockCount is 1, which indicates GetBlock
  * returned the header block (block at index 0) or if the new record is not able to fit inside the block,
  * then unpin the header, allocate a new block, reset the recordCount and add the record to the new block.
  * Else, just add the new record in the empty space of the data and update the recordCount. Lastly, just
  * dirty unpin and destroy the block
  **/
  CALL_BF(BF_GetBlock(file_handle, hp_info->blockCount - 1, block));
  void *data = BF_Block_GetData(block);
  hp_info->totalRecords++;
  
  if (hp_info->blockCount == 1 || sizeof(record) > BF_BLOCK_SIZE - (sizeof(Record) * hp_info->recordCount) - sizeof(data)) {
    CALL_BF(BF_UnpinBlock(block));

    hp_info->recordCount = 1;
    CALL_BF(BF_AllocateBlock(file_handle, block));

    data = BF_Block_GetData(block);
    Record* rec = data;
    rec[0] = record;

    hp_info->blockCount++;

  } else {

    Record* rec = data;
    rec[hp_info->recordCount - 1] = record;
    
    hp_info->recordCount++;
  }
  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return 1;
}


HeapFileIterator HeapFile_CreateIterator(int file_handle, HeapFileHeader* header_info, int id)  {

  BF_Block *headerBlock;
  BF_Block_Init(&headerBlock);

  HeapFileIterator out;
  out.idToSearch = id;
  out.file_handle = file_handle;
  out.hpInfo = (HeapFileHeader*)malloc(sizeof(HeapFileHeader));

  BF_GetBlock(file_handle, 0, headerBlock);
  void* data = BF_Block_GetData(headerBlock);

  out.hpInfo = data;
  out.hpInfo->blockCount = header_info->blockCount;
  out.hpInfo->recordCount = header_info->recordCount;
  out.hpInfo->totalRecords = header_info->totalRecords;
  
  BF_Block_SetDirty(headerBlock);
  BF_UnpinBlock(headerBlock);
  BF_Block_Destroy(&headerBlock);

  return out;
}


int HeapFile_GetNextRecord(HeapFileIterator* heap_iterator, Record** record)  {
  BF_Block* blockIterate;
  BF_Block_Init(&blockIterate);

  // TODO: Figure out how to keep all the records in memory because they cannot be accessed outside the insertRecord.

  void* data;
  int foundId = 0;
  data = BF_Block_GetData(blockIterate);
  Record* rec = data;
  * record = (Record*)malloc(sizeof(Record));
  CALL_BF(BF_GetBlock(heap_iterator->file_handle, 1, blockIterate));
  // for(int i = 1; i < heap_iterator->hpInfo->totalRecords; ++i){

  //   CALL_BF(BF_GetBlock(heap_iterator->file_handle, i, blockIterate));
  //   data = BF_Block_GetData(blockIterate);
  //   rec = data;
  //   // *Μάλλον όχι απτό 0 αλλά από την θέση του record στο block.
  //   // !Πώς είμαι σίγουρος ότι δεν θα επιστρέψει το ίδιο το block (Skip one loop?)
  //   // TODO τσέκαρε ότι σίγουρα λειτουργεί
  //   for(int j = 0 ; j < sizeof(rec)/sizeof(rec[0]); j++){
  //     if (rec[j].id == (**record).id) {
  //       foundId = 1;
  //       break;
  //     } 
  //   }

  //   if (foundId) {
      
  //     break;
  //   }

  // }

  // BF_Block_Destroy(&blockIterate);

  // if(foundId)
  //   return 1;
  // else
  //   return 0;
  * record=NULL;
  return 1;
}

