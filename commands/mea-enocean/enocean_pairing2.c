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


int16_t callback(uint8_t *data, uint16_t l_data, uint32_t addr, void *callbackdata)
{
   int16_t nerr = 0;

   if(data[4]==ENOCEAN_EVENT)
   {
      fprintf(stderr,"ENOCEAN_EVENT ...\n");
      if(data[6]==2) // SA_CONFIRM_LEARN
      {
         fprintf(stderr,"   SA_CONFIRM_LEARN ...\n");
         fprintf(stderr,"      PM Priority       : %02x", data[7]);
         fprintf(stderr,"      manufacturer ID   : %02x...%02x", data[8] & 0b00000111, data[9]);
         fprintf(stderr,"      EEP               : %02x-%02x-%02x\n", data[10], data[11], data[12]);
         fprintf(stderr,"      RSSI              : %d", data[13]);
         fprintf(stderr,"      PM Candidat ID    : %02x-%02x-%02x-%02x", data[14],data[15],data[16],data[17]);
         fprintf(stderr,"      Smart Ack ClientID: %02x-%02x-%02x-%02x", data[18],data[19],data[20],data[21]);

//         enocean_sa_confirm_learn_response(ed, 25 /* ms */, 0, &nerr);
      }
   }
   else if(data[4]==ENOCEAN_RADIO_ERP1)
   {
      fprintf(stderr,"ENOCEAN_RADIO_ERP1 (%x ...\n", addr);
   }
   else // quand on ne sais pas analyser le packet, on le dump
   {
      for(int i=0; i<l_data; i++)
      {
         if(i && (i % 10) == 0)
            fprintf(stderr, "\n");
         fprintf(stderr, "0x%02x ", data[i]);
      }
      fprintf(stderr, "\n");
   }
}


enocean_ed_t *ed=NULL;

int main(int argc, char *argv[])
{
   char *dev; // ="/dev/ttyS0";
   int16_t enocean_fd = 0;
   int16_t ret;
 
   if(argc!=2)
   {
      fprintf(stderr, "usage : %s /dev/<device>\n", argv[0]);
      exit(1);
   }
 
   dev = argv[1];
 
   uint8_t data[0xFFFF]; // taille maximum d'un packet ESP3 = 65536
   uint16_t l_data;
   int16_t nerr = 0;

   ed = enocean_new_ed();
   enocean_fd = enocean_init(ed, dev);

   if(enocean_fd<0)
   {
      fprintf(stderr,"enocean_init error: ");
      perror("");
      exit(1);
   }

   enocean_set_data_callback(ed, callback);

//   ret=enocean_learning_onoff(ed, 1, &nerr);
   ret=enocean_sa_learning_onoff(ed, 1, &nerr);

   if(ret==0)
   {   
      fprintf(stderr,"WAIT 60 SECONDS\n"); 
      int i=0; 
      for(i=0;i<60;i++)
      {
         fprintf(stderr,"%d\n", 60-i);
         sleep(1);
      }
//      enocean_learning_onoff(ed, 0, &nerr);
      enocean_sa_learning_onoff(ed, 0, &nerr);
   }
   else
      fprintf(stderr,"CAN'T LEARN\n");

   enocean_close(ed);
   enocean_free_ed(ed);
}

