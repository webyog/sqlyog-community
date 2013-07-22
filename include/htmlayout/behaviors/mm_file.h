//|
//|
//| mm_file.h
//|
//| Copyright (c) 2003
//| Andrew Fedoniouk - andrew@terrainformatica.com
//|
//| CONTENT:
//| class <B>mm_file</B> - access to memory mapped file
//| template table<RECORD> - persistent flat table
//|
//|

#ifndef __tl_mm_file_h
#define __tl_mm_file_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

#include "aux-slice.h"

namespace aux
{

  class mm_file 
  {
      HANDLE hfile;
      HANDLE hmap;

    protected:
      bool read_only;
      void*  ptr;
      size_t length;
    public:
      mm_file(): hfile(0),hmap(0),ptr(0),length(0),read_only(true) {}
      virtual ~mm_file() { close(); }

      void *open(const wchar_t *path, bool to_write = false);
      void  close();

      void * data() { return ptr; }
      size_t size() { return length; }
      void   size(size_t sz) { assert(!read_only); length = sz; }

      aux::bytes bytes() { return aux::bytes((byte*)ptr,length); }
  };
  
  inline void *mm_file::open(const wchar_t *path, bool to_write)
  {
      read_only = !to_write;

      hfile = INVALID_HANDLE_VALUE;
      hmap = INVALID_HANDLE_VALUE;
      ptr = 0;
     
      hfile = CreateFileW(path, GENERIC_READ | (read_only? 0: GENERIC_WRITE), FILE_SHARE_READ | (read_only? 0: FILE_SHARE_WRITE), NULL,
        read_only?OPEN_EXISTING:CREATE_ALWAYS, 
        FILE_ATTRIBUTE_NORMAL, NULL);
      if (hfile != INVALID_HANDLE_VALUE) 
      {
          length = GetFileSize(hfile, 0);
          hmap = CreateFileMapping(hfile, NULL, read_only? PAGE_READONLY : PAGE_READWRITE, 0, read_only?0:0x10000000, NULL);
      }
      else
      {
        DWORD erno = GetLastError();
        printf("ERROR: mm file open <%s> failed %x\n",path, erno);
        return 0;
      }
          
      if (hfile != INVALID_HANDLE_VALUE && hmap == NULL)
      {
          close();
          return 0;
      }
      else
      {
          if (hmap != NULL)
            ptr = MapViewOfFile(hmap, read_only? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);
          if(ptr == 0)
          {
            DWORD erno = GetLastError();
            printf("ERROR: map file %x\n", erno);
            close();
          }
      }
      return ptr;
  }

  inline void mm_file::close()
  {
    if (hfile && hmap && ptr) {

        if(!read_only && length)
          if (!FlushViewOfFile(ptr, length)) 
          { 
              printf("Could not flush memory to disk.\n"); 
          } 

        UnmapViewOfFile(ptr);
        ptr = 0;
    }
    if (hmap) {
        CloseHandle(hmap);
        hmap = 0;
    }
    if (hfile != INVALID_HANDLE_VALUE)
    {
        if(!read_only && length)
        {
          SetFilePointer(hfile,length,0,FILE_BEGIN);
          SetEndOfFile(hfile);
        }
        CloseHandle(hfile);
        hfile = 0;
    }
  }
  

}

#endif