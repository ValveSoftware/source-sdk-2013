/* 
7zMain.c
Test application for 7z Decoder
LZMA SDK 4.43 Copyright (c) 1999-2006 Igor Pavlov (2006-06-04)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "7zCrc.h"
#include "7zIn.h"
#include "7zExtract.h"

typedef struct _CFileInStream
{
  ISzInStream InStream;
  FILE *File;
} CFileInStream;

#ifdef _LZMA_IN_CB

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc;
  if (maxRequiredSize > kBufferSize)
    maxRequiredSize = kBufferSize;
  processedSizeLoc = fread(g_Buffer, 1, maxRequiredSize, s->File);
  *buffer = g_Buffer;
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#else

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = fread(buffer, 1, size, s->File);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#endif

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
}

void PrintError(char *sz)
{
  printf("\nERROR: %s\n", sz);
}

int main(int numargs, char *args[])
{
  CFileInStream archiveStream;
  CArchiveDatabaseEx db;
  SZ_RESULT res;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;

  printf("\n7z ANSI-C Decoder 4.43  Copyright (c) 1999-2006 Igor Pavlov  2006-06-04\n");
  if (numargs == 1)
  {
    printf(
      "\nUsage: 7zDec <command> <archive_name>\n\n"
      "<Commands>\n"
      "  e: Extract files from archive\n"
      "  l: List contents of archive\n"
      "  t: Test integrity of archive\n");
    return 0;
  }
  if (numargs < 3)
  {
    PrintError("incorrect command");
    return 1;
  }

  archiveStream.File = fopen(args[2], "rb");
  if (archiveStream.File == 0)
  {
    PrintError("can not open input file");
    return 1;
  }

  archiveStream.InStream.Read = SzFileReadImp;
  archiveStream.InStream.Seek = SzFileSeekImp;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  InitCrcTable();
  SzArDbExInit(&db);
  res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
  if (res == SZ_OK)
  {
    char *command = args[1];
    int listCommand = 0;
    int testCommand = 0;
    int extractCommand = 0;
    if (strcmp(command, "l") == 0)
      listCommand = 1;
    if (strcmp(command, "t") == 0)
      testCommand = 1;
    else if (strcmp(command, "e") == 0)
      extractCommand = 1;

    if (listCommand)
    {
      UInt32 i;
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        CFileItem *f = db.Database.Files + i;
        printf("%10d  %s\n", (int)f->Size, f->Name);
      }
    }
    else if (testCommand || extractCommand)
    {
      UInt32 i;

      /*
      if you need cache, use these 3 variables.
      if you use external function, you can make these variable as static.
      */
      UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
      Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
      size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

      printf("\n");
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        size_t offset;
        size_t outSizeProcessed;
        CFileItem *f = db.Database.Files + i;
        if (f->IsDirectory)
          printf("Directory ");
        else
          printf(testCommand ? 
            "Testing   ":
            "Extracting");
        printf(" %s", f->Name);
        if (f->IsDirectory)
        {
          printf("\n");
          continue;
        }
        res = SzExtract(&archiveStream.InStream, &db, i, 
            &blockIndex, &outBuffer, &outBufferSize, 
            &offset, &outSizeProcessed, 
            &allocImp, &allocTempImp);
        if (res != SZ_OK)
          break;
        if (!testCommand)
        {
          FILE *outputHandle;
          UInt32 processedSize;
          char *fileName = f->Name;
          size_t nameLen = strlen(f->Name);
          for (; nameLen > 0; nameLen--)
            if (f->Name[nameLen - 1] == '/')
            {
              fileName = f->Name + nameLen;
              break;
            }
            
          outputHandle = fopen(fileName, "wb+");
          if (outputHandle == 0)
          {
            PrintError("can not open output file");
            res = SZE_FAIL;
            break;
          }
          processedSize = fwrite(outBuffer + offset, 1, outSizeProcessed, outputHandle);
          if (processedSize != outSizeProcessed)
          {
            PrintError("can not write output file");
            res = SZE_FAIL;
            break;
          }
          if (fclose(outputHandle))
          {
            PrintError("can not close output file");
            res = SZE_FAIL;
            break;
          }
        }
        printf("\n");
      }
      allocImp.Free(outBuffer);
    }
    else
    {
      PrintError("incorrect command");
      res = SZE_FAIL;
    }
  }
  SzArDbExFree(&db, allocImp.Free);

  fclose(archiveStream.File);
  if (res == SZ_OK)
  {
    printf("\nEverything is Ok\n");
    return 0;
  }
  if (res == SZE_OUTOFMEMORY)
    PrintError("can not allocate memory");
  else     
    printf("\nERROR #%d\n", res);
  return 1;
}
