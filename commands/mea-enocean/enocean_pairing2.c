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


int16_t learning_state = 0;

int16_t enocean_exist(uint32_t addr)
{
   return 0;
}


int16_t learning_callback(uint8_t *data, uint16_t l_data, uint32_t addr, void *callbackdata)
{
   int16_t nerr = 0;

   enocean_ed_t *ed=(enocean_ed_t *)callbackdata;

   if(learning_state == 0)
   {
//      fprintf(stderr,"Not in learning mode\n");
      return -1;
   }

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

         enocean_sa_confirm_learn_response(ed, 1000 /* ms */, 0, &nerr);
      }
   }
   else if(data[4]==ENOCEAN_RADIO_ERP1)
   {
      fprintf(stderr,"ENOCEAN_RADIO_ERP1 (%08x ...\n", addr);
      switch(data[6])
      {
         case ENOCEAN_RPS_TELEGRAM:
         {
         /* Exemple "Trame d'un bouton"
            00 0x55  85 Synchro
            01 0x00   0 Header1 Data Lenght
            02 0x07   7 Header2 Data Lenght
            03 0x07   7 Header3 Optionnal lenght
            04 0x01   1 Header4 Packet type
            05 0x7A 122 CRC8H
            06 0xF6 246 Data1 RORG
            07 0x30  48 Data2 = B00110000
            08 0x00   0 Data3 ID1
            09 0x25  37 Data4 ID2
            10 0x86 134 Data5 ID3
            11 0x71 113 Data6 ID4
            12 0x30  48 Data7 Status
            13 0x02   2 Optionnal1 Number sub telegram
            14 0xFF 255 Optionnal2 Destination ID
            15 0xFF 255 Optionnal3 Destination ID
            16 0xFF 255 Optionnal4 Destination ID
            17 0xFF 255 Optionnal5 Destination ID
            18 0x2D  45 Optionnal6 Dbm
            19 0x00   0 Optionnal7 Security level
            20 0x38  56 CRC8D
         */
            fprintf(stderr, "=== RPS Telegram (F6) ===\n");
            fprintf(stderr, "Adresse    : %02x-%02x-%02x-%02x\n", data[8], data[9], data[10], data[11]);

            learning_state = 0;
         }
         break;
 
         case ENOCEAN_UTE_TELEGRAM:
         {
         /*
            00 0x55  85 Synchro
            01 0x00   0 Header1 Data Lenght 
            02 0x0d  13 Header2 Data Lenght
            03 0x07   7 Header3 Optionnal lenght
            04 0x01   1 Header4 Packet type
            05 0xfd 253 CRC8H
            06 0xd4 212 RORG : UTE
            07 0xa0 160 (0b10100000)
            08 0x02   2 Number of indifidual channel to be taught in
            09 0x46  70 Manufacturer-ID (8LSB)
            10 0x00   0 Manufacturer-ID (3MSB)
            11 0x12  18 TYPE
            12 0x01   1 FUNC
            13 0xd2 210 RORG
            14 0x01   1 ID1
            15 0x94 148 ID2
            16 0xc9 201 ID3
            17 0x40  64 ID4
            18 0x00   0 (status)
            19 0x01   1 (number sub telegram)
            20 0xFF 255 Optionnal2 Destination ID
            21 0xFF 255 Optionnal3 Destination ID
            22 0xFF 255 Optionnal4 Destination ID
            23 0xFF 255 Optionnal5 Destination ID
            24 0x3d  61 Optionnal6 Dbm
            25 0x00   0 Optionnal7 Security level
            26 0xde 222 CRC8D
         */
            char *operationStr = "???";
            char *responseStr  = "???";
            char *requestStr   = "???";
            uint8_t operation  = (data[7] & 0b10000000) >> 7;
            uint8_t response   = (data[7] & 0b01000000) >> 6;
            uint8_t request    = (data[7] & 0b00110000) >> 4;
            uint8_t cmnd       = (data[7] & 0b00001111);
            uint32_t addr = enocean_calc_addr(data[14], data[15], data[16], data[17]);

            uint8_t resp_request   = 0;
            uint8_t resp_operation = 0;
            switch(operation)
            {
               case 0:
                 operationStr = "Unidirectional";
                 break;
               case 1:
                 operationStr = "Bidirectional";
                 break;
            }
            resp_operation = operation << 7;

            switch(response)
            {
               case 0:
                 responseStr = "";
                 break;
               case 1:
                 responseStr = "No";
                 break;
            }
        
            switch(request)
            {
               case 0:
                 requestStr = "Teach-in request";
                 resp_request = 0b01;
                 break;
               case 1:
                 requestStr = "Teach-in deletion request";
                 resp_request = 0b10;
                 break;
               case 2:
                 requestStr = "Teach-in or deletion of teach-in, not specified";
                 if(enocean_exist(addr)==0)
                    resp_request = 0b01;
                 else
                    resp_request = 0b10;
                 break;
               case 3:
                 requestStr = "Not used";
                 break;
            }
            resp_request = resp_request << 4;

            fprintf(stderr, "=== UTE Telegram (D4) ===\n");
            fprintf(stderr, "Adresse    : %02x-%02x-%02x-%02x\n", data[14], data[15], data[16], data[17]);
            fprintf(stderr, "EEP        : %02x-%02x-%02x\n", data[13], data[12], data[11]);
            fprintf(stderr, "nb channels: %d\n", data[8]);
            fprintf(stderr, "op         : %u (%s communication)\n", operation, operationStr);
            fprintf(stderr, "rs         : %u (%s EEP Teach-In-Response message expected)\n", response, responseStr);
            fprintf(stderr, "rq         : %u (%s)\n", request, requestStr);
            fprintf(stderr, "cmnd       : %u\n", cmnd);

            if((request  != 3) &&
               (response == 0) &&
               (cmnd     == 0))
            {
               int16_t nerr = -1;

               char resp[7];
            
               resp[0]=resp_operation+resp_request+1; // DB_6
               resp[1]=data[8];  // DB_5 (nb channel)
               resp[2]=data[9];  // DB_4 (manufacturer-ID LSB)
               resp[3]=data[10]; // DB_3 (manufacturer-ID MSB)
               resp[4]=data[11]; // DB_2 (TYPE)
               resp[5]=data[12]; // DB_1 (FUNC)
               resp[6]=data[13]; // DB_0 (RORG)

               int ret=enocean_send_radio_erp1_packet(ed, ENOCEAN_UTE_TELEGRAM, ed->id, 0, addr, resp, 7, 0, &nerr);
               fprintf(stderr,"RESPONSE = %d\n", ret);

               learning_state = 0;
            }
         }
         break;
      } 
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

   enocean_set_data_callback2(ed, learning_callback, ed);

//   ret=enocean_learning_onoff(ed, 1, &nerr);
   ret=enocean_sa_learning_onoff(ed, 1, &nerr);
   learning_state = 1;
   if(ret==0)
   {   
      fprintf(stderr,"WAIT 60 SECONDS\n"); 
      int i=0; 
      for(i=0;i<60 && learning_state == 1;i++)
      {
         fprintf(stderr,"%d\n", 60-i);
         sleep(1);
      }
//      enocean_learning_onoff(ed, 0, &nerr);
      learning_state = 0;
      enocean_sa_learning_onoff(ed, 0, &nerr);
   }
   else
      fprintf(stderr,"CAN'T LEARN\n");

   enocean_close(ed);
   enocean_free_ed(ed);
}

