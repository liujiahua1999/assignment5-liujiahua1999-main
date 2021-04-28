#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"



static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;
//num_queries, total times read
//with num_hits to calculate the hit rate

int num_entry_insert = 0;
//////////////////////// PART FOR CACHE ESSENTIAL, CREATE & DESTROY ////////////////////////////////////////////////
int cache_create(int num_entries) 
{
  if (num_entries < 2)
  {
    return -1;
  }

  if (num_entries > 4096)
  {
    return -1;
  }

  if(cache == NULL)
  {
    cache_size = num_entries;
    cache = calloc(num_entries,sizeof(cache_entry_t));
    // cache initialize
    for (int i = 0; i < num_entries; i++)
    {
      cache[i].valid=false;
      // mark cache unused
    }
    return 1;
  }

  return -1;
}


//help function to clear the cache
int null_cache(cache_entry_t **cache){*cache = NULL;return 0;}
int cache_destroy(void) 
{
  if(cache == NULL)
  {
    return -1;
  }
  
  if (cache != NULL)
  {
    null_cache(&cache);
    cache_size = 0;
    clock = 0;
    num_entry_insert = 0;
    return 1;
  }
  return -1;
}

/////////////////////////////// PART FOR CACHE MODIFICATION, READ & WRITE ///////////////////////////////////////////


int cache_lookup(int disk_num, int block_num, uint8_t *buf) 
{


  if(cache == NULL)
    {return -1;}
  if(disk_num >= 16)
    {return -1;}
  if(disk_num <= -1)
    {return -1;}
  if(block_num >= 256)
    {return -1;}
  if(block_num <= -1)
    {return -1;}
  if(cache == NULL)
    {return -1;}
  if(buf == NULL)
    {return -1;}
  if (num_entry_insert == 0)
    {return -1;}
  //Loopup happenning when block num and disk num matches, and num_entry_insert != 0
  num_queries++;

  for (int i = 0; i < cache_size; ++i)
  {
    if (cache[i].block_num == block_num && cache[i].disk_num == disk_num)
    {
      memcpy(buf,cache[i].block,JBOD_BLOCK_SIZE);
      num_hits++;
      clock++;
      cache[i].access_time = cache[i].access_time + clock;
      
      return 1;
    }
    
  }
  return -1;
}




int cache_insert(int disk_num, int block_num, const uint8_t *buf) 
{

    if(disk_num >= 16)
  {return -1;}
    if(disk_num <= -1)
  {return -1;}
    if(block_num >= 256)
  {return -1;}
    if(block_num <= -1)
  {return -1;}
    if(cache == NULL)
  {return -1;}
    if(buf == NULL)
  {return -1;}

  //Check if cache is zero out, which cause bugs that makes program think there is a cache for disk 0 & block 0
  for (int i = 0; i < cache_size; i++)
  {
    if(cache[i].block_num == block_num && cache[i].disk_num == disk_num)
    {
      if (memcmp(buf,cache[i].block,JBOD_BLOCK_SIZE)==0)
        {return  -1;}
      else
        {memcpy(cache[i].block,buf,JBOD_BLOCK_SIZE);}
    }
  }

  int min = cache[0].access_time;
  int index = 0;
  //find the entry that used least if full, imply LRU
  if (num_entry_insert == cache_size)
  {
    for (int i = 0; i < cache_size; ++i)
    {
      
      if (cache[i].access_time < min)
      {
        min = cache[i].access_time;
        index = i;
      }
    }

    if (cache[index].access_time == min)
    {
            

      cache[index].block_num = block_num;
      cache[index].disk_num = disk_num;

      cache[index].valid = true;
      cache[index].access_time = 0;
      memcpy(cache[index].block,buf,JBOD_BLOCK_SIZE);
    
      return 1;
      
    }
    return -1;
  }

  //inset new entry
  for (int i = 0; i < cache_size; ++i)
  {  

    if (cache[i].valid==false)
    {

        cache[i].block_num = block_num;
        cache[i].disk_num = disk_num;
        cache[i].valid = true;
        cache[i].access_time = 0;

        memcpy(cache[i].block,buf,JBOD_BLOCK_SIZE);
        num_entry_insert++;
        cache[i].access_time++;

        return 1;
      
    }

  }
  return -1;

}




void cache_update(int disk_num, int block_num, const uint8_t *buf) 
{   

  for (size_t i = 0; i < cache_size; i++)
  { 
    if(cache[i].block_num == block_num && cache[i].disk_num == disk_num)
      {
        if (memcmp(buf,cache[i].block,JBOD_BLOCK_SIZE)==0)
          {
            return  -1;
          }
        else
          {
            memcpy(cache[i].block,buf,JBOD_BLOCK_SIZE);
            return 1;
          }  
      }
  }

}




void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}