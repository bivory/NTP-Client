#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>


/*
 * See http://www.ietf.org/rfc/rfc5905.txt
 *     Section A.1.2.  Packet Data Structures
 * and https://www.meinbergglobal.com/english/info/ntp-packet.htm
 */
#define NTP_VERSION     3
#define NTP_MODE_CLIENT 0x3

#define NTP_HEADER_VERSION_SET(_header, _x) (_header |= ((_x & 0x7) << 0))
#define NTP_HEADER_MODE_SET(_header, _x) (_header |= ((_x & 0x7) << 3))

typedef struct
{
   uint32_t seconds;
   uint32_t fraction;
} ntp_timestamp;

typedef struct
{
   uint32_t       header;
   uint32_t       root_delay;
   uint32_t       root_dispersion;
   uint32_t       reference_identifier;
   ntp_timestamp  reference_timestamp;
   ntp_timestamp  originate_timestamp;
   ntp_timestamp  receive_timestamp;
   ntp_timestamp  transmit_timestamp;
} ntp_rsp_msg;

void print_words(size_t num_words, uint32_t buffer[num_words])
{
   size_t   word_i;

   for(word_i = 0; word_i < num_words; word_i++)
   {
      printf("%2lu: %#x\n", word_i, buffer[word_i]);
   }
}

int main(void)
{
   const int            ntp_port = 123;
   /* IP returned by pool.ntp.org: */
   const char *         ntp_host = "66.228.59.187";
   int                  ntp_socket;
   struct sockaddr_in   server;
   ntp_rsp_msg          ntp_req_msg;
   int                  receive_len;
   ntp_rsp_msg          ntp_rsp_msg;
   long                 seconds;


   ntp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if (ntp_socket == -1)
   {
      perror("Failed to create socket");
      return EXIT_FAILURE;
   }

   memset(&server, 0, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_port = htons(ntp_port);
   server.sin_addr.s_addr = inet_addr(ntp_host);
   if (server.sin_addr.s_addr == INADDR_NONE)
   {
      fprintf(stderr, "Failed to resolve hostname %s\n", ntp_host);
      return EXIT_FAILURE;
   }

   memset(&ntp_req_msg, 0, sizeof(ntp_req_msg));
   NTP_HEADER_VERSION_SET(ntp_req_msg.header, NTP_VERSION);
   NTP_HEADER_MODE_SET(ntp_req_msg.header, NTP_MODE_CLIENT);
   printf("Sending NTP request to %s:%d\n", ntp_host, ntp_port);
   if (sendto(ntp_socket,
              &ntp_req_msg, sizeof(ntp_req_msg),
              0,
              (struct sockaddr *) &server, sizeof(server)) == -1)
   {
      perror("Failed to send NTP request");
      return EXIT_FAILURE;
   }

   printf("Waiting for NTP response...\n");
   receive_len = recvfrom(ntp_socket,
                          &ntp_rsp_msg, sizeof(ntp_rsp_msg),
                          0,
                          NULL, NULL);
   if (receive_len <= 0)
   {
      perror("Failed to receive NTP response");
      return EXIT_FAILURE;
   }

   printf("Received %d bytes\n", receive_len);
   print_words(receive_len/sizeof(uint32_t), (uint32_t *)&ntp_rsp_msg);

   seconds = ntohl(ntp_rsp_msg.transmit_timestamp.seconds);
   printf("Transmit seconds: %lu\n", seconds);

   close(ntp_socket);
   ntp_socket = -1;

   return EXIT_SUCCESS;
}
