#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;


const char *hostname = "localhost";
#define PORT 4756


int connexion(SOCKET *s, char *hostname, uint32 port)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   if(sock == -1)
   {
      VERBOSE(2) perror("socket()");
      return 1;
   }

   struct hostent *hostinfo = NULL;
   SOCKADDR_IN sin = { 0 }; /* initialise la structure avec des 0 */

   hostinfo = gethostbyname(hostname); /* on récupère les informations de l'hôte auquel on veut se connecter */
   if (hostinfo == NULL) /* l'hôte n'existe pas */
   {
      VERBOSE(2) fprintf (stderr, "Unknown host %s.\n", hostname);
      return 1;
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr; /* l'adresse se trouve dans le champ h_addr de la structure hostinfo */
   sin.sin_port = htons(PORT); /* on utilise htons pour le port */
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == -1)
   {
      VERBOSE(2) perror("connect()");
      return 1;
   }
   
   *s=socket;
   return 0;
}


int envoie(SOCKET *s, char *message)
{
   if(send(sock, message, strlen(message), 0) < 0)
   {
      perror("send()");
      return errno;
   }
   return 0;
}


monitor()
{
   int exit=0;
   SOCKET nodejs_socket=-1;
   char message[1024];
   
   do
   {
     if(connexion(&nodejs_socket, hostname, PORT)==0)
     {
       do
       {
         sleep(5); // un check toutes les 5 secondes
         
         // récupération des données
         
         // formatage des données
         
         ret = envoie(&nodejs_socket, message);
       }
       while(ret==0);
       close(nodejs_socket);
     }
     else
     {
       VERBOSE(5) fprintf(stderr,"Retry next time ...\n");
       sleep(5); // on essayera de se reconnecter dans 5 secondes
     }
   }
   while(exit==0);
}


