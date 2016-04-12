#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/time.h>
#include <limits.h>
#include <fcntl.h>
#include <termios.h>

#include "enocean.h"


enocean_ed_t *ed=NULL;

int main(int argc, char *argv[])
{
   char *dev; // ="/dev/ttyS0";
   int16_t enocean_fd = 0;

   char *addr;
   char *type;
   int  channel;
   int  value;

   char data[3];
 
   int16_t ret;

   if(argc!=6)
   {
      fprintf(stderr, "usage : %s /dev/<device> addr type channel value\n", argv[0]);
      exit(1);
   }
 
   dev = argv[1];
   addr = argv[2];
   type = argv[3];
   channel = atoi(argv[4]);
   value = atoi(argv[5]);

   int a,b,c,d, n;

   ret=sscanf(addr,"%2x-%2x-%2x-%2x%n", &a, &b, &c, &d, &n);
   if(ret!=4 || n!=strlen(addr))
   {
      fprintf(stderr,"Addess error (%s)\n", addr);
      exit(1);
   }
   uint32_t _addr = enocean_calc_addr(a, b, c, d);

   if(strcmp(type,"d2")!=0 && strcmp(type,"D2")!=0)
   {
      fprintf(stderr,"unknown type (%s)\n", type);
      exit(1);
   }

   ret=sscanf(argv[4],"%d%n\n",&channel, &n);
   if(ret!=1 || n!=strlen(argv[4]) || channel < 0)
   {
      fprintf(stderr,"channel error (%s)\n", argv[4]);
      exit(1);
   }

   ret=sscanf(argv[5],"%d%n\n",&value, &n);
   if(ret!=1 || n!=strlen(argv[5]) || value < 0 || value > 100)
   {
      fprintf(stderr,"value error (%s)\n", argv[5]);
      exit(1);
   }

   int16_t nerr = 0;
   ed = enocean_new_ed();
   enocean_fd = enocean_init(ed, dev);

   if(enocean_fd<0)
   {
      fprintf(stderr,"enocean_init error: ");
      perror("");
      exit(1);
   }

   data[0]=0x01;
   data[1]=channel && 0b11111;
   data[2]=value && 0b1111111;

   ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_VLD_TELEGRAM, ed->id, 0, _addr, data, 3, 0, &nerr);
   fprintf(stderr,"RESPONSE = %d\n", ret);

   enocean_close(ed);
   enocean_free_ed(ed);
}

