// a renommer en enocenapairing.c

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

/*
Appairage Micro-module on/off UBID1008 1 contact 2A

1- Appuyer sur le bouton LRN du mico-module pendant 5 sec. jusqu’à ce que le relai bascule régulièrement
   à chaque seconde.
2- Pendant ce temps, faites 1 appui sur la touche de l’émetteur ou du capteur choisi pour l’appairer au
   micro-module. Le récepteur valide l’association en maintenant sur ON le relai pendant 2,5 sec.
3- Dès que le basculement du relais redémarre, appuyez alors sur la touche LRN du micro-module pendant
   0,5 sec pour terminer l’opération.
4- Pour dissocier émetteur et récepteur, procéder de la même manière.
5- Pour effacer tous les appairages en une seule opération, appuyer 10 secondes sur le bouton
   CLR du micro-module
*/

int16_t build_erp1_rps_broadcast_packet(uint32_t source, uint8_t data, uint8_t status, uint8_t *packet, uint16_t l_packet)
{
   uint16_t ptr=0;
   uint8_t crc8h = 0, crc8d = 0;
   
   if(l_packet < 21)
      return -1;

   // Octet de synchro
   packet[0] = 0x55;
   
   // longueur données
   packet[1] = 0;
   crc8h = proc_crc8(crc8h, packet[1]);
   packet[2] = 7;
   crc8h = proc_crc8(crc8h, packet[2]);
   
   // longueur données optionnelles  
   packet[3] = 7;
   crc8h = proc_crc8(crc8h, packet[3]);

   // type de packet
   packet[4] = ENOCEAN_RADIO_ERP1;
   crc8h = proc_crc8(crc8h, packet[4]);

   // CRC8H
   packet[5] = crc8h;
         
   // rorg
   packet[6] = ENOCEAN_RPS_TELEGRAM;
   crc8d = proc_crc8(crc8d, packet[6]);

   packet[7] = data;
   crc8d = proc_crc8(crc8d, packet[7]);

   // ajout adresse source
   uint8_t a,b,c,d;
   uint32_t addr = source;
 
   d=addr & 0xFF;
   addr = addr >> 8;
   c=addr & 0xFF;
   addr = addr >> 8;
   b=addr & 0xFF;
   addr = addr >> 8;
   a=addr & 0xFF;
         
   packet[8] = a;
   crc8d = proc_crc8(crc8d, packet[8]);
   packet[9] = b;
   crc8d = proc_crc8(crc8d, packet[9]);
   packet[10] = c;
   crc8d = proc_crc8(crc8d, packet[10]);
   packet[11] = d;
   crc8d = proc_crc8(crc8d, packet[11]);

   // ajout status
   packet[12] = status;
   crc8d = proc_crc8(crc8d, packet[12]);
   
   // données optionnelles
   // num subtelegram
   packet[13] = 3;
   crc8d = proc_crc8(crc8d, packet[13]);

   // ajout adresse destination = broadcast
   packet[14] = 0xFF;
   crc8d = proc_crc8(crc8d, packet[14]);
   packet[15] = 0xFF;
   crc8d = proc_crc8(crc8d, packet[15]);
   packet[16] = 0xFF;
   crc8d = proc_crc8(crc8d, packet[16]);
   packet[17] = 0xFF;
   crc8d = proc_crc8(crc8d, packet[17]);

   // RSSI
   packet[18] = 0xFF;
   crc8d = proc_crc8(crc8d, packet[18]);   
   
   // security level
   packet[19] = 0;
   crc8d = proc_crc8(crc8d, packet[19]);
   
   packet[20] = crc8d;
   
   return 21;
}


int main(int argc, char *argv[])
{
 int16_t enocean_fd = 0;
 char *dev;
 uint32_t addr;
 uint8_t packet[21]; // taille maximum d'un packet ESP3/ERP1/RPS = 21
 uint8_t response[64];
 uint16_t l_response = 0;
 int16_t nerr = 0;
 int ret=0;
 
   if(argc!=2)
   {
      fprintf(stderr,"usage : %s /dev/<device>\n",argv[0]);
      exit(1);
   }
   else
   {
      dev = argv[1];
   }
 
   enocean_ed_t *ed = enocean_new_ed();
   enocean_fd = enocean_init(ed, dev);
   if(enocean_fd<0)
   {
      fprintf(stderr,"enocean_init error: ");
      perror("");
      exit(1);
   }

   if(enocean_get_baseid(ed, &addr, &nerr) != -1)
   {
      // data: AI appuyé, status: T21=1, NU=1
      if(build_erp1_rps_broadcast_packet(addr,0b00010000, 0b00001100, packet, sizeof(packet))!=-1)
      {
         l_response=sizeof(response);
         if(enocean_send_packet(ed, packet, sizeof(packet), response, &l_response, &nerr)<0)
         {
            fprintf(stderr,"enocean_send_packet error: %d\n", nerr);
            ret=1;
            goto main_clean_exit;
         }
      }
      else
      {
         fprintf(stderr,"can't build packet\n");
         ret=1;
         goto main_clean_exit;
      }

      sleep(1);

      // data: AI relaché, status: T21=1, NU=0
      if(build_erp1_rps_broadcast_packet(addr, 0b00000000, 0b00000100, packet, sizeof(packet))!=-1)
      {
         l_response=sizeof(response);
         if(enocean_send_packet(ed, packet, sizeof(packet), response, &l_response, &nerr)<0)
         {
            fprintf(stderr,"enocean_send_packet error: %d\n", nerr);
            ret=1;
            goto main_clean_exit;
         }
      }
      else
      {
         fprintf(stderr,"can't build packet\n");
         ret=1;
         goto main_clean_exit;
      }
   }
   else
   {
      fprintf(stderr,"enocean_get_baseid error: %d\n", nerr);
      ret=1;
//      goto main_clean_exit;
   }
   
main_clean_exit:
   enocean_close(ed);
   enocean_free_ed(ed);
   exit(ret);
}

