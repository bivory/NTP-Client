#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MSG_BUFFER_SZ 256

/*
 * See http://www.ietf.org/rfc/rfc5905.txt
 * and https://www.meinbergglobal.com/english/info/ntp-packet.htm
 */
typedef struct
{
   uint32_t seconds;
   uint32_t fraction;
} ntp_timestamp;

typedef struct
{
   uint32_t       root_delay;
   uint32_t       root_dispersion;
   uint32_t       reference_identifier;
   ntp_timestamp  reference_timestamp;
   ntp_timestamp  originate_timestamp;
   ntp_timestamp  receive_timestamp;
   ntp_timestamp  transmit_timestamp;
   uint32_t       key_id;
   uint32_t       message_digest;
} ntp_ntp_rsp_msg;

void print_bytes(size_t num_bytes, uint8_t buffer[num_bytes])
{
   size_t   byte_i;

   for(byte_i = 0; byte_i < num_bytes; byte_i++)
   {
      printf("%2lu: %#x\n", byte_i, buffer[byte_i]);
   }
}

int main(void)
{
   const int            ntp_port = 123;
   /* IP returned by pool.ntp.org: */
   const char *         ntp_host = "66.228.59.187";
   int                  ntp_socket;
   struct sockaddr_in   server;
   uint8_t              ntp_req_msg[48] = { 010, 0, 0, 0, 0, 0, 0, 0, 0 };
   int                  receive_len;
   ntp_ntp_rsp_msg      ntp_rsp_msg;
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

   printf("Sending NTP request to %s:%d\n", ntp_host, ntp_port);
   if (sendto(ntp_socket,
              ntp_req_msg, sizeof(ntp_req_msg),
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
#if 0
   print_bytes(receive_len, receive_buffer);
#endif
   seconds = ntohl(ntp_rsp_msg.transmit_timestamp.seconds);
   printf("Transmit seconds: %lu\n", seconds);

   close(ntp_socket);
   ntp_socket = -1;

   return EXIT_SUCCESS;
}
