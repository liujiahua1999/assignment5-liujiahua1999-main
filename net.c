#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
int num_read = 0;
while (num_read < len)
{
  int r = read(fd,&buf[num_read], len - num_read);
  if (r < 0)
  {
    return false;
  }
  if (r >= 0)
  {
    num_read += r;
    return true;
  } 
}
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
int num_write = 0;
while (num_write < len)
{
  int w = write(fd,&buf[num_write], len - num_write);
  if (w < 0)
  {
    return false;
  }
  if (w >= 0)
  {
    num_write += w;
    return true;
  }
  
}
return true;
}

static bool send_packet(int sd, uint32_t op, uint8_t *block) {
int cmd = op >> 26;

uint8_t header_write[HEADER_LEN+JBOD_BLOCK_SIZE];
uint8_t OP_BIG_ENDIAN[4];
  OP_BIG_ENDIAN[0] = op >> 24;
  OP_BIG_ENDIAN[1] = op >> 16;
  OP_BIG_ENDIAN[2] = op >> 8;
  OP_BIG_ENDIAN[3] = op;

header_write[6] = 0;
header_write[7] = 0;  

//uint16_t len_short;
//uint16_t len_long;
uint16_t len;


  memcpy(&header_write[2],OP_BIG_ENDIAN,4);

  if(cmd == JBOD_WRITE_BLOCK)
  {
	 memcpy(&header_write[HEADER_LEN], block, JBOD_BLOCK_SIZE);
	len = HEADER_LEN+JBOD_BLOCK_SIZE;
  header_write[0] = ntohs(len);
  header_write[1] = ntohs(len) >> HEADER_LEN;
  }
  else
  {
  len = HEADER_LEN;

  header_write[0] = ntohs(len);
  header_write[1] = ntohs(len) >> HEADER_LEN;
  }


//network order
//BIG Endian


  if(!nwrite(cli_sd , len, header_write))
  {
	  return false;
  }
  return true;
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  //Create socket
  
  cli_sd =socket(PF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1)
  {
    return false;
  }

  //convert ip to binary form
  struct sockaddr_in caddr;
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);
  if (inet_aton(ip,&caddr.sin_addr) == 0)
  {
    return false;
  }

  //connect
  if (connect(cli_sd,(const struct sockaddr *)&caddr,sizeof(caddr)) == -1)
  {
    return false;
  }
  return true;
}


void jbod_disconnect(void) {
  //close connection
  close(cli_sd);
  cli_sd = -1;

}

/* sends the JBOD operation to the server and receives and processes the
 * response. */

int jbod_client_operation(uint32_t op, uint8_t *block) {

int cmd = op >> 26;
uint8_t header_read[HEADER_LEN+JBOD_BLOCK_SIZE];

uint16_t len;

 send_packet(cli_sd,op,block);
  


  if(cmd == JBOD_READ_BLOCK || cmd == JBOD_SIGN_BLOCK)
  {
	  len = HEADER_LEN+JBOD_BLOCK_SIZE;
  header_read[0] = ntohs(len);
  header_read[1] = ntohs(len) >> HEADER_LEN;//network order
  }
  else
  {
    len = HEADER_LEN;
  header_read[0] = ntohs(len);
  header_read[1] = ntohs(len) >> HEADER_LEN;
  }
  if(!nread(cli_sd , len, header_read))
  {
	  return -1;
  }

  if(cmd == JBOD_READ_BLOCK || cmd == JBOD_SIGN_BLOCK)
  {
memcpy(block, &header_read[HEADER_LEN], JBOD_BLOCK_SIZE);
  }

/* attempts to send a packet to sd; returns true on success and false on
 * failure */

return 1;
}