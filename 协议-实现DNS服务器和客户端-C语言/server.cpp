#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dns_protocol.h"

#define MAX_CLIENTS 100
#define BUFFER_SZ 256

static _Atomic unsigned int cli_count = 0;
static int uid = 0;
const char *port = "53";
unsigned char *ip = (unsigned char *)"127.0.0.1";

struct sockaddr_in serv_addr;

typedef struct {
  //	Struct for each client
  struct sockaddr_in address;
  int sockfd;
  int uid;
  char name[BUFFER_SZ];
} client;
client *clients[MAX_CLIENTS];

unsigned char *getaddr(unsigned char domain[]) {
  /*
    input: domain name
    output: IP address
  */
  int length = strlen((const char *)domain);
  char *res = NULL;
  char length_str[4];
  strncpy(res, "0.0.0.", strlen("0.0.0."));
  sprintf(length_str, "%d", length);
  strcat(res, length_str);
  printf("IP address = %s\n", res);
  return (unsigned char *)res;
}

int judge(int n) {
  int count = 0;
  if (n == 0) return 1;
  while (n != 0) {
    n /= 10;
    count++;
  }
  return count;
}

void ChangetoDnsNameFormat(unsigned char *dns, unsigned char *host) {
  int lock = 0, i;
  strcat((char *)host, ".");

  for (i = 0; i < int(strlen((char *)host)); i++) {
    if (host[i] == '.') {
      *dns++ = i - lock;
      for (; lock < i; lock++) {
        *dns++ = host[lock];
      }
      lock++;
    }
  }
  *dns++ = '\0';
}

void getDomainName(unsigned char *qname,
                   unsigned char *dname) {  // DNS name format to domain format
  unsigned int pos = 0;
  *dname = '\0';

  if (*qname == 0) {
    *dname = '.';
  } else {
    while (*qname != 0) {
      pos = *qname;
      for (int j = 0; j < (int)pos; j++) {
        *dname++ = *(++qname);
      }
      *dname++ = '.';
      qname++;
    }
  }
}

void send_message(void *arg) {
  // Parse messages
  unsigned char buffer[BUFFER_SZ];
  // char str[INET_ADDRSTRLEN];
  client *cli = (client *)arg;
  int length, j, name_len;
  socklen_t len = sizeof(struct sockaddr_in);

  recvfrom(cli->sockfd, (char *)buffer, BUFFER_SZ, 0,
           (struct sockaddr *)&cli->address, (socklen_t *)&len);

  struct DNS_HEADER *header = (struct DNS_HEADER *)&buffer;

  header->rd = 1;
  header->tc = 0;
  header->aa = 0;
  header->opcode = 0;
  header->qr = 1;
  header->rcode = 0;
  header->cd = 0;
  header->ad = 0;
  header->z = 0;
  header->ra = 1;
  header->q_count = htons(1);
  header->ans_count = htons(1);
  header->auth_count = 0;
  header->add_count = 0;
  unsigned char *qname = (unsigned char *)(buffer + sizeof(struct DNS_HEADER));
  QUERY query;
  query.name = (unsigned char *)(buffer + sizeof(struct DNS_HEADER));
  name_len = strlen((const char *)query.name);
  
  // recieve from client
  unsigned char dname[256];
  bzero(dname, 256);
  getDomainName(query.name, dname);
  printf("QUERY Domain Name: %s, length: %lu\n", dname, strlen((const char *)dname));

  char IPv4[32];
  char length_str[4];
  memset(IPv4, 0, 32);
  strncpy(IPv4, "0.0.0.", strlen("0.0.0."));
  sprintf(length_str, "%d", name_len - 1);
  strcat(IPv4, length_str);
  if (name_len == 0) {
    memset(IPv4, 0, 32);
    strncpy(IPv4, "0.0.0.1", strlen("0.0.0.1"));
  }
  printf("RES Ip: %s\n", IPv4);
  // unsigned char *name = (unsigned char *)(buffer + recv);
  struct RES_RECORD record;
  unsigned char *ans = (unsigned char *)(buffer + sizeof(struct DNS_HEADER) +
                                         sizeof(struct QUESTION) +
                                         (int)strlen((const char *)qname) + 1);
  int offset_pointer = htons(0xc00c);
  memcpy(ans, &(offset_pointer), 2);

  record.name = (unsigned char *)(buffer + sizeof(struct DNS_HEADER));
  record.resource = (struct R_DATA *)(ans + 2);
  record.rdata = (unsigned char *)(ans + 2 + sizeof(struct R_DATA));
  record.resource->type = htons(T_A);
  record.resource->data_len = htons(4);
  record.resource->_class = htons(1);
  record.resource->ttl = htons(1);

  record.resource = (struct R_DATA *)(ans + 2);
  record.resource->type = htons(T_A);
  record.resource->_class = htons(1);
  record.resource->ttl = htons(1);

  struct sockaddr_in temp;
  inet_pton(AF_INET, (const char *)IPv4, &temp.sin_addr.s_addr);
  memcpy(record.rdata, &temp.sin_addr.s_addr, ntohs(record.resource->data_len));

  // inet_pton(AF_INET, IPv4, record.rdata);

  // printf("%s\n", IPv4);
  length = sizeof(struct DNS_HEADER) + strlen((const char *)qname) + 1 +
           sizeof(struct R_DATA) + sizeof(struct QUESTION) + 4 + 2;

  j = sendto(cli->sockfd, (char *)buffer, length, 0,
             (struct sockaddr *)&cli->address, len);
  printf("Sent RES Length = %d\n\n", j);
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  int sockfd;

  struct sockaddr_in cli_addr;

  if (argc == 2) {
    port = argv[1];
  }

  sockfd = socket(PF_INET, SOCK_DGRAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr((const char *)ip);
  serv_addr.sin_port = htons(atoi(port));

  //	Now bind the host address using bind() call
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }

  printf("welcome!\n");
  while (1) {
    //	Check if max clients is reached
    if ((cli_count + 1) == MAX_CLIENTS) {
      printf("Max clients reached !");
      continue;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr((const char *)ip);
    serv_addr.sin_port = htons(atoi(port));

    //  Client config
    client *cli = (client *)malloc(sizeof(client));
    cli->address = cli_addr;
    cli->sockfd = sockfd;
    cli->uid = uid++;

    send_message(cli);

    sleep(1);
  }

  close(sockfd);

  return 0;
}
