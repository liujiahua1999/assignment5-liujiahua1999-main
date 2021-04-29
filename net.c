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
  int n = read(fd,&buf[num_read], len - num_read);
  if (n < 0)
  {
    return false;
  }
  if (n = 0)
  {
    return true;
  }
  if (n > 0)
  {
    num_read += n;

  } 
}
return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
int num_write = 0;
while (num_write < len)
{
  int n = write(fd,&buf[num_write], len - num_write);
  if (n < 0)
  {
    return false;
  }
  if (n = 0)
  {
    return true;
  }
  if (n > 0)
  {
    num_write += n;

  }
  
}
return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  
  uint16_t len;
  uint8_t header[HEADER_LEN];

  if (!nread(fd,HEADER_LEN,header))
  {
    return false;

  }


  int offset = 0;
  memcpy(&len, header + offset, sizeof(len));
  offset += sizeof(len);
  memcpy(op, header + offset, sizeof(*op));
  offset += sizeof(*op);
  memcpy(ret, header + offset, sizeof(*ret));
    len = ntohs(len);
  *op = ntohl(*op);
  *ret = ntohs(*ret);


return true;
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
  uint32_t cmd = op >> 26;
  if(cmd == JBOD_WRITE_BLOCK)
  {
        uint16_t len = 8;
  uint8_t header[HEADER_LEN];


  len = ntohs(len);
  op = ntohl(op);

  int offset = 0;
  memcpy(header + offset,&len, sizeof(len));
  offset += sizeof(len);
  memcpy(header + offset, op, sizeof(op));

  if (!nwrite(sd,HEADER_LEN,header))
  {
    return false;

  }

  return true;
  }
  
  if(cmd == JBOD_READ_BLOCK)
  {
        uint16_t len = 264;
  uint8_t header[HEADER_LEN];


  len = ntohs(len);
  op = ntohl(op);

  int offset = 0;
  memcpy(header + offset,&len, sizeof(len));
  offset += sizeof(len);
  memcpy(header + offset, op, sizeof(op));
  offset += sizeof(op);
  memcpy(header + offset, *block, sizeof(*block));
  if (!nwrite(sd,HEADER_LEN,header))
  {
    return false;

  }
  return true;
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


/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  //close connection
  close(cli_sd);
  cli_sd = -1;

}

/* sends the JBOD operation to the server and receives and processes the
 * response. */

int jbod_client_operation(uint32_t op, uint8_t *block) {
  //loop 
//op =
  uint32_t cmd = op >> 26;
  if(cmd == JBOD_READ_BLOCK)
  {
  uint16_t len = 264;
  uint8_t header[HEADER_LEN];


  len = ntohs(len);
  op = ntohl(op);

  int offset = 0;
  memcpy(header + offset,&len, sizeof(len));
  offset += sizeof(len);
  memcpy(header + offset, op, sizeof(op));
  offset += sizeof(op);
  memcpy(header + offset, block, sizeof(block));
    if (!nwrite(cli_sd,HEADER_LEN,header))
    {
      return false;
    }
  return true;
  }

  else
  {
  uint16_t len = 8;
  uint8_t header[HEADER_LEN];


  len = ntohs(len);
  op = ntohl(op);

  
  memcpy(header,len, 2);
  
  memcpy(header + 2, op, 4);

  if (!nwrite(cli_sd,HEADER_LEN,header))
  {return false;}

  return true;
  }
  

return true;

    //if packet size does include block (is long)

    //is this the right way to write the code?

}
