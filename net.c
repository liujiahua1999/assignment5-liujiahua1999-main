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
  if (recv(cli_sd, buf, len, 0) < 0) {

		return false;
	}
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  if (send(fd, buf, len, 0) < 0) {

		return false;
	}
  return true;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {

}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  struct sockaddr_in server;
  fprintf(stderr, "connected to the JBOD server at 127.0.0.1\n");

  cli_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == cli_sd) {

    return -1;
	}

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(3333);

	if (connect(cli_sd, (struct sockaddr *)&server, sizeof(server)) < 0) {

		return -1;
	}
  return 1;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  close(cli_sd);
  cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {


  uint32_t cmd = op >> 26;
  uint32_t disk_num = op & 0x3FFFFFF >> 22;
  uint32_t block_num = (op & 0xFF);

  //printf("jbod_client_operation,op:%x, cmd:%d, disk_num:%d, block_num:%d\n", op, cmd, disk_num, block_num);

  char buf[264] = {0};
  char buf2[264] = {0};


  buf[2] = (op >> 24) & 0xFF;
  buf[3] = (op >> 16) & 0xFF;
  buf[4] = (op >> 8) & 0xFF;
  buf[5] = op & 0xFF;
  uint32_t len = 8;


  if(cmd == JBOD_WRITE_BLOCK)
  {
	 memcpy(buf+8, block, 256);
	 len = 264;
  }

  buf[0] = htons(len) & 0xFF;
  buf[1] = (htons(len) >> 8) & 0xFF;

  if(true != nwrite(cli_sd, len, buf))
  {
	  printf("net write nok\n");
  }
  len = 8;

  if(cmd == JBOD_READ_BLOCK || cmd == JBOD_SIGN_BLOCK)
  {
	  len = 264;
  }

  buf2[0] = htons(len) & 0xFF;
  buf2[1] = (htons(len) >> 8) & 0xFF;
  if(true != nread(cli_sd, len, buf2))
  {
	  printf("net read nok\n");
  }

  if(cmd == JBOD_READ_BLOCK || cmd == JBOD_SIGN_BLOCK)
  {
	  //printf("%s\n", buf2);
	  memcpy(block, buf2+8, 256);
  }


   return true;






}
