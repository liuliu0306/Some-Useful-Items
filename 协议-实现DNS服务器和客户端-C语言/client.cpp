#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "dns_protocol.h"

#define LENGTH 256
#define BUF_SIZE 256
int sockfd = 0;
struct sockaddr_in serv_addr;

unsigned char *ip = (unsigned char *)"127.0.0.1";
const char *port = "53";

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

int send_msg_handler(unsigned char *dns_name) {
  unsigned char buffer[LENGTH];
  struct DNS_HEADER *header = (struct DNS_HEADER *)&buffer;
  header = (struct DNS_HEADER *)&buffer;
  // Get values
  header->id = (unsigned short)htons(getpid());
  header->qr = 0;
  header->opcode = 0;
  header->aa = 0;
  header->tc = 0;
  header->rd = 1;
  header->ra = 0;
  header->z = 0;
  header->ad = 0;
  header->cd = 0;
  header->rcode = 0;
  header->q_count = htons(1);
  header->ans_count = 0;
  header->auth_count = 0;
  header->add_count = 0;

  unsigned char *qname = (unsigned char *)(buffer + sizeof(struct DNS_HEADER));
  ChangetoDnsNameFormat(qname, dns_name);

  struct QUESTION *question =
      (struct QUESTION *)(qname + 1 + strlen((const char *)qname));
  question = (struct QUESTION *)(qname + strlen((const char *)qname) + 1);

  question->qtype = htons(T_A);
  question->qclass = htons(1);

  // printf("qname = %s\n", qname);
  // strncpy((char *)qname, dns_name, strlen(dns_name));
  // header->add_count = strlen((const char *)qname) + 1;

  int length = int(sizeof(struct DNS_HEADER)) + strlen((const char *)qname) +
               1 + int(sizeof(struct QUESTION));

  int respo = sendto(sockfd, (char *)buffer, length, 0,
                     (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (respo < 0) {
    printf("Send Failed!\n");
  }

  return respo;
}

void recv_msg_handler(int from_send) {
  unsigned char buffer[LENGTH];
  char str[INET_ADDRSTRLEN];  // for ipv4

  int length, ans_count;
  socklen_t len = sizeof(struct sockaddr_in);
  printf("hello\n");
  recvfrom(sockfd, (char *)buffer, LENGTH, 0, (struct sockaddr *)&serv_addr,
           (socklen_t *)&len);

  struct DNS_HEADER *header = (struct DNS_HEADER *)buffer;
  unsigned char *residual = (unsigned char *)(buffer + from_send + 2);
  struct RES_RECORD record;

  ans_count = ntohs(header->ans_count);

  while (ans_count--) {
    // Direct
    record.resource = (struct R_DATA *)residual;
    if (ntohs(record.resource->type) == T_A) {
      record.rdata = (unsigned char *)(residual + sizeof(struct R_DATA));
      inet_ntop(AF_INET, record.rdata, str, sizeof(str));
      printf("Response: %s\n", str);
    }

    // Redirect
    length = sizeof(struct R_DATA) + ntohs(record.resource->data_len) + 2;
    residual += length;
  }
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  unsigned char *dns_name = (unsigned char *)argv[1];
  // if (argv[2]) ip = (unsigned char *)argv[2];
  // if (argv[3] && (strcmp(argv[3], "SHELL=/bin/bash") != 0)) port = argv[3];

  if (argc == 2) exit(1);

  if (argc == 3) {
    if (strcmp((const char *)argv[2], "localhost") != 0)
      ip = (unsigned char *)argv[2];
  }

  if (argc == 4) {
    if (strcmp((const char *)argv[2], "localhost") != 0)
      ip = (unsigned char *)argv[2];
    port = argv[3];
  }

  //	Socket config
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr((const char *)ip);
  serv_addr.sin_port = htons(atoi(port));

  struct timeval tv_out;
  tv_out.tv_sec = 2;
  tv_out.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out)) <
      0) {
    perror("Failed!");
    exit(1);
  }
  // client_handler((unsigned char *)domainName);
  int send_from = send_msg_handler(dns_name);
  recv_msg_handler(send_from);

  close(sockfd);
  return 0;
}
