#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <assert.h>

#include "jbod.h"
#include "mdadm.h"
#include "util.h"
#include "tester.h"
#include "net.h"
//////////////////////// PART FOR DISK ESSENTIAL, MOUNT & UNMOUNT ////////////////////////////////////////////////
uint8_t cache_block[JBOD_BLOCK_SIZE];
//translate jbod operation to bits, which operate harddisk
uint32_t encode_operation(jbod_cmd_t cmd, int disk_num, int block_num)
{
    int op = cmd << 26  | disk_num << 22 | block_num;
    return op;
}


int mdadm_mount(void) 
{
  uint32_t op = encode_operation(JBOD_MOUNT,0,0); 

  if (jbod_client_operation(op,NULL) == 1)
    {return 1;}
  else
    {return -1;}
}


int mdadm_unmount(void) 
{
  uint32_t op = encode_operation(JBOD_UNMOUNT,0,0); 

  if (jbod_client_operation(op,NULL) == 1)
    {return 1;}
  else
    {return -1;}
}

//////////////////////// PART FOR DISK MODIFICATION, READ & WRITE ////////////////////////////////////////////////


int mdadm_read(uint32_t addr,  uint32_t len , uint8_t *buf) 
{
  
  if(jbod_client_operation(encode_operation(JBOD_ALREADY_MOUNTED,0,0),NULL) == -1)
    {return -1;}

  if (addr + len > JBOD_NUM_DISKS * JBOD_DISK_SIZE)
  {return -1;}

  if (len == 0)
    {return 0;}

  if (len > 1024)
    {return -1;}

  if(len&&buf == NULL)
    {return -1;}


  int curr_addr = addr;
  int Byte_Been_Read = 0;
  int Not_firstblock = 0;

//SEEK
  int disk_num = curr_addr/JBOD_DISK_SIZE;
  int block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
  int offset = (curr_addr % JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;


  int lookup_cache = cache_lookup(disk_num,block_num,cache_block);



  while (curr_addr < addr + len)
  {
    //keep going til the end

    uint8_t whole_block[JBOD_BLOCK_SIZE]={0};

    if (Not_firstblock == 0)
    {

      if(offset+len <= JBOD_BLOCK_SIZE )
      //if read within block, start and end at same block
      {
        lookup_cache = cache_lookup(disk_num,block_num,cache_block);

        if(lookup_cache == -1)
        {
          jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
          jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);
          jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);
          memcpy(buf,&whole_block[offset],len);
          cache_insert(disk_num,block_num,whole_block);
        }    

        if(lookup_cache == 1)
        {
          memcpy(buf,&cache_block[offset],len);
        }

        return len;
      }


      if(offset+len > JBOD_BLOCK_SIZE)
      
      {
        lookup_cache = cache_lookup(disk_num,block_num,cache_block);
        disk_num = curr_addr/JBOD_DISK_SIZE;
        block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
        offset = (curr_addr % JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;

        if(lookup_cache == -1)
        {
          jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
          jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);
          jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);
          memcpy(buf,whole_block+offset,JBOD_BLOCK_SIZE-offset);
          cache_insert(disk_num,block_num,whole_block);
        }

        if(lookup_cache == 1)
        {
          memcpy(buf,cache_block+offset,JBOD_BLOCK_SIZE-offset);
        }

        curr_addr += (JBOD_BLOCK_SIZE-offset);
        Not_firstblock = 1;
        Byte_Been_Read+=JBOD_BLOCK_SIZE-offset;
      }
    }



    if (len-Byte_Been_Read>JBOD_BLOCK_SIZE)
    //if more than one block remaining
    {
      disk_num = curr_addr/JBOD_DISK_SIZE;
      block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
      lookup_cache = cache_lookup(disk_num,block_num,cache_block);

      if(lookup_cache == -1)
      {
        jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
        jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);
        jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);
        memcpy(&buf[Byte_Been_Read],whole_block,JBOD_BLOCK_SIZE);}

      if(lookup_cache == 1)
      {
        memcpy(&buf[Byte_Been_Read],cache_block,JBOD_BLOCK_SIZE);
      }

      curr_addr += JBOD_BLOCK_SIZE;
      Byte_Been_Read += JBOD_BLOCK_SIZE;
      cache_insert(disk_num,block_num,whole_block);
          
    }



    if (len-Byte_Been_Read<=JBOD_BLOCK_SIZE)
    //if last block
    {
      disk_num = curr_addr/JBOD_DISK_SIZE;
      block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
      lookup_cache = cache_lookup(disk_num,block_num,cache_block);

        
        
      if(lookup_cache == -1)
      {
        jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
        jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);
        jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);
        memcpy(&buf[Byte_Been_Read],whole_block,len-Byte_Been_Read);
      }

      if(lookup_cache == 1)
      {
        memcpy(&buf[Byte_Been_Read],cache_block,len-Byte_Been_Read);
      }

      cache_insert(disk_num,block_num,whole_block);
      
      return len;
    
    }
  }
  //TO AVOID ENDLESS LOOP
  return len;
}



int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) 
{
  uint32_t op1 = encode_operation(JBOD_ALREADY_MOUNTED,0,0); 
  if(jbod_client_operation(op1,NULL) == -1)
    {return -1;}

  if (addr + len > JBOD_NUM_DISKS * JBOD_DISK_SIZE)
    {return -1;}

  if (len == 0)
    {return 0;}

  if (len > 1024)
    {return -1;}

  if(len&&buf == NULL)
    {return -1;}


  int curr_addr = addr;
  int Written_byte = 0;

  int disk_num = curr_addr/JBOD_DISK_SIZE;
  int block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
  int offset = (curr_addr % JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;

  int lookup_cache = cache_lookup(disk_num,block_num,cache_block);

  uint8_t written_block[JBOD_BLOCK_SIZE];
  uint8_t whole_block[JBOD_BLOCK_SIZE];

  jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
  jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);
  jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);

  memcpy(written_block,whole_block,JBOD_BLOCK_SIZE);
  //seek again because READ is being executed, which move to next block
  jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
  jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);


  if (offset+len < JBOD_BLOCK_SIZE)
  {

    lookup_cache = cache_lookup(disk_num,block_num,cache_block);

    if (lookup_cache == 1)
    {
      memcpy(&written_block[offset],buf,len);
      jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
      cache_update(disk_num,block_num,written_block);
    }

    if (lookup_cache == -1)
    {
      memcpy(&written_block[offset],buf,len);
      jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
      cache_insert(disk_num,block_num,written_block);
    }

    return len;
  }

  while (curr_addr < addr + len)
  {

    uint8_t written_block[JBOD_BLOCK_SIZE];
    uint8_t whole_block[JBOD_BLOCK_SIZE];

    jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);
    memcpy(written_block,whole_block,JBOD_BLOCK_SIZE);
    jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
    jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);


    if (Written_byte+offset < JBOD_BLOCK_SIZE)
    {
      lookup_cache = cache_lookup(disk_num,block_num,cache_block);
      disk_num = curr_addr/JBOD_DISK_SIZE;
      block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
      offset = (curr_addr % JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;
      jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
      jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);

      if (lookup_cache == -1)
      { 
        memcpy(&written_block[offset],buf,JBOD_BLOCK_SIZE-offset);
        jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
        cache_insert(disk_num,block_num,written_block);
      }


      if (lookup_cache == 1)
      {
        memcpy(&written_block[offset],buf,JBOD_BLOCK_SIZE-offset);
        jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
        cache_update(disk_num,block_num,written_block);
      }

      Written_byte+=(JBOD_BLOCK_SIZE-offset);
      curr_addr+=(JBOD_BLOCK_SIZE-offset);

      block_num+=1;
      offset =0;

    }


    if (len-Written_byte>JBOD_BLOCK_SIZE)
    {
      lookup_cache = cache_lookup(disk_num,block_num,cache_block);
      disk_num = curr_addr/JBOD_DISK_SIZE;
      block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
      offset = (curr_addr % JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;

      jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
      jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);

      if(lookup_cache == -1)
      {
        memcpy(written_block,&buf[Written_byte],JBOD_BLOCK_SIZE);
        jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
        cache_insert(disk_num,block_num,whole_block);
      }

      if(lookup_cache == 1)
      {
        memcpy(written_block,&buf[Written_byte],JBOD_BLOCK_SIZE);
        jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
        cache_update(disk_num,block_num,whole_block);
      }
      curr_addr += JBOD_BLOCK_SIZE;
      Written_byte += JBOD_BLOCK_SIZE;
      block_num+=1;

    }


    if (len-Written_byte<=JBOD_BLOCK_SIZE)
    {
      lookup_cache = cache_lookup(disk_num,block_num,cache_block);
      disk_num = curr_addr/JBOD_DISK_SIZE;
      block_num = (curr_addr%JBOD_DISK_SIZE)/JBOD_BLOCK_SIZE;
      offset = (curr_addr % JBOD_DISK_SIZE)%JBOD_BLOCK_SIZE;
      jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
      jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);

      jbod_client_operation(encode_operation(JBOD_READ_BLOCK,0,0),whole_block);
      memcpy(written_block,whole_block,JBOD_BLOCK_SIZE);

      jbod_client_operation(encode_operation(JBOD_SEEK_TO_DISK,disk_num,0),NULL);
      jbod_client_operation(encode_operation(JBOD_SEEK_TO_BLOCK,0,block_num),NULL);
          

      if(lookup_cache == -1)
      {
        memcpy(written_block,&buf[Written_byte],len-Written_byte);
        jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
        cache_insert(disk_num,block_num,whole_block);
      }

      if(lookup_cache == 1)
      {
        memcpy(written_block,&buf[Written_byte],len-Written_byte);
        jbod_client_operation(encode_operation(JBOD_WRITE_BLOCK,0,0),written_block);
        cache_update(disk_num,block_num,whole_block);
      }

      return len;

    }

  }

  //AVOID ENDLESS LOOP
  printf("Failed\n");
  return len;
}