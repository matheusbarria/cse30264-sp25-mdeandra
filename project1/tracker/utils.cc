/* utils.c : Helpful utilities and output
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

void dump_sockaddr_in (struct sockaddr_in * pInfo)
{
   /*
   struct sockaddr_in {
      short int          sin_family;  // Address family, AF_INET
      unsigned short int sin_port;    // Port number
      struct in_addr     sin_addr;    // Internet address
      unsigned char      sin_zero[8]; // Same size as struct sockaddr
   };
   */

   printf("Dumping the sockaddr_in struct at %p\n", pInfo);
   printf("  sin_family: 0x%04X\n", pInfo->sin_family);
   printf("  sin_port (Host Order):   0x%04X -> %d\n", pInfo->sin_port, pInfo->sin_port);

   // Show this as flipped (endian-wise)
   uint16_t    theCorrectedPort;

   // Invoke htons - host to network short
   theCorrectedPort = htons(pInfo->sin_port);
   printf("  sin_port (Net Order):    0x%04X -> %d\n", theCorrectedPort, theCorrectedPort);

   uint32_t    theAddress;
   memcpy(&theAddress, &pInfo->sin_addr, 4);
   printf("  sin_addr:   0x%04X -> ", theAddress);

   /* Print it out one byte at a time - note that there are other mechanisms */
   uint8_t * pPtr;

   pPtr = (uint8_t *) &(pInfo->sin_addr);

   for(int j=0; j<4; j++)
   {
      printf("%d", *(pPtr+j));

      if(j<3)
      {
         printf(".");
      }
   }

   printf("\n");
}
