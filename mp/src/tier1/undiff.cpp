//========= Copyright Valve Corporation, All rights reserved. ============//
//
// UnDiff - Apply difference block
//
//=============================================================================//

#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/diff.h"
#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void ApplyDiffs(uint8 const *OldBlock, uint8 const *DiffList,
                int OldSize, int DiffListSize, int &ResultListSize,uint8 *Output,uint32 OutSize)
{
  uint8 const *copy_src=OldBlock;
  uint8 const *end_of_diff_list=DiffList+DiffListSize;
  uint8 const *obuf=Output;
  while(DiffList<end_of_diff_list)
  {
    //  printf("dptr=%x ",DiffList-d);
    uint8 op=*(DiffList++);
    if (op==0)
    {
      uint16 copy_sz=DiffList[0]+256*DiffList[1];
      int copy_ofs=DiffList[2]+DiffList[3]*256;
      if (copy_ofs>32767)
        copy_ofs|=0xffff0000;
    //    printf("long cp from %x to %x len=%d\n", copy_src+copy_ofs-OldBlock,Output-obuf,copy_sz);
      
      memcpy(Output,copy_src+copy_ofs,copy_sz);
      Output+=copy_sz;
      copy_src=copy_src+copy_ofs+copy_sz;
      DiffList+=4;
    }
    else
    {
      if (op & 0x80)
      {
        int copy_sz=op & 0x7f;
        int copy_ofs;
        if (copy_sz==0)
        {
          copy_sz=DiffList[0];
          if (copy_sz==0)
          {
            // big raw copy
            copy_sz=DiffList[1]+256*DiffList[2]+65536*DiffList[3];
            memcpy(Output,DiffList+4,copy_sz);
    //          printf("big rawcopy to %x len=%d\n", Output-obuf,copy_sz);
            
            DiffList+=copy_sz+4;
            Output+=copy_sz;
          }
          else
          {
            copy_ofs=DiffList[1]+(DiffList[2]*256);
            if (copy_ofs>32767)
              copy_ofs|=0xffff0000;
    //          printf("long ofs cp from %x to %x len=%d\n", copy_src+copy_ofs-OldBlock,Output-obuf,copy_sz);
            
            memcpy(Output,copy_src+copy_ofs,copy_sz);
            Output+=copy_sz;
            copy_src=copy_src+copy_ofs+copy_sz;
            DiffList+=3;
          }
        }
        else
        {
          copy_ofs=DiffList[0];
          if (copy_ofs>127)
            copy_ofs|=0xffffff80;
    //        printf("cp from %x to %x len=%d\n", copy_src+copy_ofs-OldBlock,Output-obuf,copy_sz);
          
          memcpy(Output,copy_src+copy_ofs,copy_sz);
          Output+=copy_sz;
          copy_src=copy_src+copy_ofs+copy_sz;
          DiffList++;
        }
      }
      else
      {
    //      printf("raw copy %d to %x\n",op & 127,Output-obuf);
        memcpy(Output,DiffList,op & 127);
        Output+=op & 127;
        DiffList+=(op & 127);
      }
    }
  }
  ResultListSize=Output-obuf;
  
}
