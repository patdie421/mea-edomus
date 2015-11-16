#include <Arduino.h>
#include <AltSoftSerial.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

#define __DEBUG__ 1

// utilisation des ports (ARDUINO UNO) Dans ce sketch
/* PORTD
 * PIN 0 : PC(TX) [RX]
 * PIN 1 : PC(RX) [TX]
 * PIN 2 : - [INT] : <non utilisée>
 * PIN 3 : X [INT / PWM] : SORTIE : RESET du sim900
 * PIN 4 : X P0 - ENTREE : détection POWER UP/DOWN (LOW = DOWN, HIGH = UP), dispo en lecture seule par SMS
 * PIN 5 : X P1 [PWM] : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN 6 : X P2 [PWM] : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN 7 : X P3 - ENTREE : détection ALARME, dispo en lecture seule par SMS
 *
 * PORDB
 * PIN 8 : SIM900_RX
 * PIN 9 : SIM900_TX
 * PIN 10 : X [PWM / SS]   - ENTREE : LOW = SIM900 desactivé
 * PIN 11 : X [PWM / MOSI] - ENTREE : LOW = command vers MCU uniquement (mode configuration)
 * PIN 12 : X [MISO]       - ENTREE : LOW = interprétation par le MCU des messages non solicités desactivée (mode transparent)
 * PIN 13 : LED1 [LED / SCK] - SORTIE : led d'état - clignotement toutes les 500ms = fonctionnement nominal / toutes les 125ms = erreur, pas d'opération SIM900 possible
 * PIN NA : x
 * PIN NA : x
 *
 * PORTC
 * PIN A0 : X P4 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A1 : X P5 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A2 : X P6 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A3 : X P7 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A4 : X P8 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A5 : X P9 : ENTREE/SORTIE - dispo pour I/O SMS
 * PIN A6*: x PAS DISPO SUR UNO
 * PIN A7*: x PAS DISPO SUR UNO
 */

/*
 Interface de commandes SMS via SIM900
 
 Ce programme permet de transformer un Arduino en un module "autonome et/ou connecté" 
 capable de  contrôler 10 entrées/sorties. Relié à une interface SIM900, le module permet
 de piloter à distance des E/S en fonction d'ordres reçus par SMS. Il émet également des
 alerte SMS (POWER UP/DOWN et  ALARM ON/OFF)  lorsque l'une des deux entrées réservées à
 cet effet change d'état.
 
 Ce module dispose de :
 - 8 E/S libres, dont 6 peuvent être utilisées en entrées Analogiques et 2 en sorties PWM
 (analogiques). 
 - 2 Entrées logiques spécifiques qui permettent :
 * pour l'une de connaitre l'état d'un alimentation électrique (via un interface à 
 réaliser)
 * pour l'autre de connaitre l'état d'une alarme (interface à réaliser)
 Un traitement spécifique est associé à ces deux entrées en plus de pouvoir être
 interrogées à distance comme les 8 autres entrés/sorties. Ces entrées réagissent sur
 front pour le déclenchement d'alarmes. Les alarmes sont envoyées sous forme de SMS
 (POWER DOWN, POWER UP, ALARM ON, ALARM OFF).
 
 Les entrées/sorties sont numérotées de la façon suivante :
 
 MODULE /   ARDUINO   / Fonction
 
 0    /    PIN 4    / détection POWER UP/DOWN (LOW = DOWN, HIGH = UP), dispo en lecture
 1    /    PIN 5    / E/S dont PWN
 2    /    PIN 6    / E/S dont PWN
 3    /    PIN 7    / détection ALARME, dispo en lecture seule
 4    /    PIN A0   / E/S dont entrée analogique
 5    /    PIN A1   / E/S dont entrée analogique
 6    /    PIN A2   / E/S dont entrée analogique
 7    /    PIN A3   / E/S dont entrée analogique
 8    /    PIN A4   / E/S dont entrée analogique
 9    /    PIN A5   / E/S dont entrée analogique
 
 Les communication/réaction au SMS sont contrôlés (ACL). Le module dispose d'un carnet
 de  numéro de téléphone (10 numéros disponibles) stockés en EEPROM (commandes MCU pour
 gérer la liste). Seul les numéros contenus dans cette liste peuvent agir sur le module
 par SMS. Le carnet fait également fonction de "liste de diffusion", les alarmes sont
 transmises à tous les numéro enregistrés.
 
 Le module s'interface avec un ordinateur via :
 
 - la liaison serie (USB sur UNO ou plus généralement pin 0 et 1 d'un ATMEGA328)
 - 3 lignes de contrôle (P10, 11 et 12)
 
 Il se connecte au SIM900 via :
 - une liaison serie "logiciel" (AltSoftSerial) sur PIN 8 et 9
 - un ligne "reset" (P3) à connecter à l'entrée Reset d'un module SIM900.
 
 La LED (pin13) indique l'état de fonctionnement (clignotement lent = OK ou rapide = HS)
 
 Le module peut être contrôle depuis le SIM900 (via SMS) ou depuis la liaison Serie. De
 plus il fait fonction de passerelle (transparente) entre les deux équipements, permettant
 ainsi à l'ordinateur de recevoir et d'émettre des données de/vers le module GSM.  En
 fonctionnement normal (lignes P10, P11 et P12 à HIGH ou non connectées), tous les
 octets reçus depuis le SIM900 sont retransmis sur la ligne serie et inversement.
 Neanmoins, les données en provenance du SIM900 sont interprétés par le MCU après après
 avoir été retransmission. Les données en provenance de l'ordinateur sont retransmises
 sans interprétation par le MCU.
 
 Ce comportement peut être modifié de la façon suivante :
 
 - en mettant P10 à LOW : la retransmission des données du SIM900 vers le PC est bloqué.
 - en mettant P11 à LOW : le MCU doit interpréter les commandes en provenance du PC (les
 données ne sont alors pas transmises au SIM900). Lors de l'initialisation du module
 si la communication ne peut pas être établie dans les 10s avec un SIM900, le module
 passe automatiquement dans ce mode (les états de P10, 11 et P12 n'ont aucun effet et
 la LED (13) clignote rapidement).
 - en mettant P12 à LOW : le MCU n'interprète pas les données en provenance du SI900
 (mode transparent).
 
 - lorsque P11 à LOW ou que le SIM900 est absent, les commandes suivantes peuvent être
 transmises au MCU :
 
 <LF/CR>       - affiche le prompt (>)
 PIN:<xxxx>    - stockage du code pin xxxx de la sim en ROM
 PIN:C         - efface le code pin
 NUM:<x>,<nnnnnnnnnnnnnnnnnnn>
 - ajoute un numero de telephone dans l'annuaire du MCU (10 positions 
 disponibles, de 0 à 9 et 20 caractères maximum par numéro)
 NUM:<x>,C     - efface un numéro
 CMD:##<...>## - voir format des commandes SMS
 LST - affiche le contenu de la ROM
 
 Attention : saisissez les commandes sans espace (y compris au début et à la fin). Si le
 prompt n'est pas affiché, appuyer "entrée".
 
 Exemples :
 
 Stocker le conde PIN 1234
 > PIN:1234
 DONE
 >
 
 Effacer le code PIN
 > PIN:C
 DONE
 >
 
 Ajouter un numéro de téléphone à la position 1
 > NUM:1,+33066166XXXX
 DONE
 >
 
 Supprimer un numéro
 > NUM:1,C
 DONE
 >
 
 Liste le carnet de numéro de téléphone
 > LST
 <à faire>
 
 Changer l'état des lignes :
 PIN 0 positionnée à HIGH
 PIN 2 PWM à 100
 lecture PIN 4
 
 > CMD:##0,L,H;1,A,100;4:A:G##
 <à faire>
 
 Les commandes interprétés par le modules doivent avoir le formation suivant :
 
 ##{PIN1},{COMMANDE1},{PARAM1};{PIN2},{COMMANDE2},{PARAM2};...##
 
 avec :
 PIN       numéro de la sortie/sortie Interface_type_003 correspondance avec
 I/O Arduino (Pin Arduino/Interface_type_003) : 0:4, 1:5, 2:6, 3:7,
 4:14(A0), 5:15(A1), 6:16(A2), 7:17(A3), 8:18(A4), 9:19(A5)
 COMMANDE  action à réaliser :
 - 'L' : nouvelle valeur logique
 - 'A' : nouvelle valeur analogique
 PARAM     parametre de la commande :
 - pour 'L' => H (set high), L (set low)
 - pour 'A' => entier positif
 - pour 'L' et 'A' => 'G' = Get value : lecture d'une valeur
 
 Les réponses sont de la forme :
 <à faire><mettre les codes erreurs disponible>
 */

#ifdef __DEBUG__ > 0
prog_char lastSMS[] PROGMEM  = {
  "LAST SMS: "};
prog_char unsolicitedMsg[] PROGMEM  = {
  "CALL UNSOLICITED_MSG CALLBACK"};
void printDebugStringFromProgmem(prog_char *s)
{
  int i=0;
  while(1)
  {
    char c =  pgm_read_byte_near(s + i);
    if(c)
    {
      Serial.write(c);
    }
    else
      break;
  }
}
#endif

/******************************************************************************/
//
// Class BlinkLeds - HEADER : pour un clignotement de led synchronisé
//
/******************************************************************************/
class BlinkLeds
{
public:
  BlinkLeds(unsigned int i);
  void run();
  inline int getLedState() {
    return ledState;
  }
  void setInterval(unsigned int i);
private:
  int interval;
  int ledState;
  unsigned long previousMillis;
  inline unsigned long diffMillis(unsigned long chrono, unsigned long now)
  {
    return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
  }
};


/******************************************************************************/
//
// Class BlinkLeds - BODY
//
/******************************************************************************/
BlinkLeds::BlinkLeds(unsigned int i)
{
  interval = i;
  ledState = LOW;
  previousMillis = 0;
}


void BlinkLeds::run()
{
  unsigned long currentMillis = millis();

  if(diffMillis(previousMillis, currentMillis) > interval) {
    previousMillis = currentMillis;
    ledState = !ledState;
  }
}


void BlinkLeds::setInterval(unsigned int i)
{
  interval = i;
}


/******************************************************************************/
//
// Class Sim900 - HEADER
//
/******************************************************************************/
#define SIM900_BUFFER_SIZE 200
extern const char *SIM900_ok_str;
extern const char *SIM900_error_str;
extern const char *SIM900_standard_returns[];

class Sim900
{
public:
  Sim900();
  int sync(long timeout);
  int init();

  int echoOff();
  int echoOn();
  inline int getEchoOnOff() { 
    return echoOnOff; 
  };

  int setAtTimeout(int t) { 
    atTimeout = t; 
  };
  int setEchoTimeout(int t) { 
    echoTimeout = t; 
  };
  int setSmsTimeout(int t) { 
    smsTimeout = t; 
  };

  int waitLines(char *vals[], long timeout);
  unsigned char *readLine(long timeout);
  int waitString(char *val, long timeout);
  int readStringTo(char *str, int l_str, char *val, long timeout);

  int sendChar(char c, int isEchoOn);
  inline int sendChar(char c) { 
    return sendChar(c, echoOnOff); 
  };

  int sendCr(int isEchoOn) { 
    return sendChar('\r',isEchoOn); 
  };
  inline int sendCr() { 
    return sendCr(echoOnOff); 
  };

  int sendString(char *str, int isEchoOn);
  inline int sendString(char *str) { 
    return sendString(str, echoOnOff); 
  };

  int sendStringFromProgmem(char *str, int isEchoOn);
  inline int sendStringFromProgmem(char *str) {
    return sendStringFromProgmem(str, echoOnOff); 
  };

  int sendATCmnd(char *cmnd, int isEchoOn);
  inline int sendATCmnd(char *cmnd) { 
    return sendATCmnd(cmnd, echoOnOff); 
  };

  int sendATCmndFromProgmem(prog_char *str, int isEchoOn);
  inline int sendATCmndFromProgmem(prog_char *str) { 
    return sendATCmndFromProgmem(str, echoOnOff); 
  };

  int sendSMS(char *num, char *text, char isEchoOn);
  inline int sendSMS(char *num, char *text) { 
    return sendSMS(num, text, echoOnOff); 
  };

  int sendSMSFromProgmem(char *num, prog_char *text, char isEchoOn);
  inline int sendSMSFromProgmem(char *num, prog_char *text) { 
    return sendSMSFromProgmem(num, text, echoOnOff); 
  };

  void setPinCode(char *code);
  int run(unsigned char c);
  int (* read)(void);
  int (* write)(char c);
  int (* available)(void);
  int (* flush)(void);

  void setRead(int (*f)(void)) { 
    read=f; 
  };
  void setWrite(int (*f)(char)) { 
    write=f; 
  };
  void setAvailable(int (*f)(void)) { 
    available=f; 
  };
  void setFlush(int (*f)(void)) { 
    flush=f; 
  };

  void *userData;
  void setUserData(void *data) { 
    userData = data; 
  };
  unsigned char *getLastSMSPhoneNumber() { 
    return lastSMSPhoneNumber; 
  };

  int (* SMSCallBack)(char *, void *); // callback appeler lorsqu'un SMS est recu (première ligne)
  int (* defaultCallBack)(char *, void *);

  void setSMSCallBack(int (*f)(char *, void *)) { 
    SMSCallBack = f; 
  };
  void setDefaultCallBack(int (*f)(char *, void *)) { 
    SMSCallBack = f; 
  };

private:
  // les différents timeout
  int echoTimeout; // echo du caractère du SIM900
  int atTimeout; // timeout d'une commande AT
  int smsTimeout; // timeout de la commande d'envoie d'un SMS

  // buffers
  unsigned char buffer[SIM900_BUFFER_SIZE];
  int bufferPtr;

  // numéro de téléphone de l'expéditeur du dernier SMS recu
  unsigned char lastSMSPhoneNumber[20];

  // code pin de la carte SIM
  unsigned char pinCode[9];

  // indicateur d'écho ON/OFF
  char echoOnOff;

  // drapeaux
  int smsFlag;

  // methodes internes
  int analyseBuffer();
  inline unsigned long diffMillis(unsigned long chrono, unsigned long now) {
    return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
  };
};


/******************************************************************************/
//
// Class Sim900 - BODY
//
/******************************************************************************/
const char *SIM900_ok_str="OK";
const char *SIM900_error_str="ERROR";
const char *SIM900_standard_returns[]={
  SIM900_ok_str,SIM900_error_str,NULL};
const char *SIM900_smsPrompt="> ";

inline int _sim900_read()
/**
 * \brief     fonction de lecture par défaut d'un caractère.
 * \details   Point de lecture pour la réception des commandes/données à traiter par sim900 si aucune autre fonction n'a été positionnée par la methode setReadFunction. Lit le premier caractère du buffer d'entrée
 * \return    < 0 (-1) si pas de données disponible, le caractère lu sinon.
 */
{
  int c=Serial.read();
  return c;
}


inline int _sim900_write(char car)
/**
 * \brief     fonction d'écriture par défaut d'un caractère.
 * \param     car   caractère a insérer dans le buffer de sortie
 * \details   fonction d'écriture des résultats renvoyés par sim900 si aucune autre fonction n'a été positionnée par la methode setWriteFunction. Ajoute un caractere dans le buffer de sortie
 * \return    toujours 0
 */
{
  Serial.write(car);
  return 0;
}


inline int _sim900_available()
/**
 * \brief     fonction permettant de connaitre le nombre de caractères présent dans le buffer d'entrée.
 * \details   fonction par defaut si aucune autre fonction n'a été positionnée par la methode setAvailableFunction.
 * \return    nombre de caractères disponibles
 */
{
  return Serial.available();
}


inline int _sim900_flush()
/**
 * \brief     fonction permettant d'attendre le "vidage" du buffer de sortie.
 * \details   fonction par defaut si aucune autre fonction n'a été positionnée par la methode setFlushFunction.
 * \return    toujours 0
 */
{
  Serial.flush();
  return 0;
}


Sim900::Sim900()
{
  smsFlag = 0;

  bufferPtr = 0;
  buffer[0]=0;
  lastSMSPhoneNumber[0]=0;

  echoOnOff=1; // echo activé par defaut

  write=_sim900_write;
  read=_sim900_read;
  available=_sim900_available;
  flush=_sim900_flush;

  echoTimeout=200; // 50 ms
  atTimeout=200;
  smsTimeout=1000;

  userData = NULL;
  SMSCallBack = NULL;
}


int Sim900::echoOff()
/**
 * \brief     desactive l'écho des caractères du SIM900.
 * \details   Transmet la commande ATE0 et met à jour l'indicateur d'état (echoOnOff) en concéquence.
 * \return    résultat de l'emission de la commande AT (-1 : erreur de communication (echoTimeout), 0 = reponse "OK", 1 = reponse "ERROR").
 */
{
  if(sendATCmnd("E0",0)<0) // suppression de l'echo
    return -1;
  echoOnOff=0;
  return waitLines((char **)SIM900_standard_returns, atTimeout);
}


int Sim900::echoOn()
/**
 * \brief     active l'écho des caractères du SIM900.
 * \details   Transmet la commande ATE1 et met à jour l'indicateur d'état (echoOnOff) en concéquence.
 * \return    résultat de l'emission de la commande AT (-1 : erreur de communication (echoTimeout), 0 = reponse "OK", 1 = reponse "ERROR").
 */
{
  if(sendATCmnd("E1",0)<0)  // suppression de l'echo
    return -1;
  echoOnOff=1;
  return waitLines((char **)SIM900_standard_returns, atTimeout);
}


void Sim900::setPinCode(char *code)
/**
 * \brief     stock le pin code qui pourra être utilisé pour l'initialisation de SIM900.
 * \return    aucun retour.
 */
{
  strcpy((char *)pinCode, code);
}


int Sim900::sendSMS(char *tel, char *text, char isEchoOn)
/**
 * \brief     Emission d'un SMS "texte" var le numero de telephone indiqué.
 * \details   
 * \param     tel : chaine de caractère contenant le numéro de telephone
 * \param     text : message à emettre (140 caractères maximum)
 * \param     isEchoOn : 1 si des caractères d'echo sont attendus (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  //  char *prompt="> ";
  if(sendString("AT+CMGS=\"", isEchoOn)<0)
    return -1;
  if(sendString(tel, isEchoOn)<0)
    return -1;
  if(sendChar('"',isEchoOn)<0)
    return -1;
  //  sendString("\"", isEchoOn);
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;

  if(sendString(text, isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;

  if(sendChar(26,isEchoOn)<0) // CTRL-Z
    return -1; 

  return 0;
}


int Sim900::sendSMSFromProgmem(char *tel, prog_char *text, char isEchoOn)
/**
 * \brief     Emission d'un SMS "texte" vers le numero de telephone indiqué. Le texte du message est stocké en memoire programme (flash)
 * \param     tel   chaine de caractères contenant le numéro de telephone
 * \param     text  message à émettre (140 caractères maximum)
 * \param     isEchoOn : 1 si des caractères d'echo sont attendus (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout" ou "smsTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  //  char *prompt="> ";
  if(sendString("AT+CMGS=\"",isEchoOn)<0)
    return -1;
  if(sendString(tel,isEchoOn)<0)
    return -1;
  if(sendChar('"',isEchoOn)<0)
    return -1;
  //  sendString("\"",isEchoOn);
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;

  if(sendStringFromProgmem(text,isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;
  if(waitString((char *)SIM900_smsPrompt, smsTimeout)<0)
    return -1;

  if(sendChar(26,isEchoOn)<0) // CTRL-Z
    return -1;
}


/**
 * \brief     Emission d'un caractère vers le SIM900
 * \details   
 * \param     c : caractère à transmettre
 * \param     isEchoOn : 1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 timeout de l'écho atteint (voir setEchoTimeout), 0 OK.
 */
int Sim900::sendChar(char c, int isEchoOn)
{
  this->write(c);
  // lecture de l'echo
  if(isEchoOn)
  {
    unsigned long start = millis(); // démarrage du chrono
    while(!this->available())
    {
      unsigned long now = millis();      
      if(diffMillis(start,  now) > echoTimeout)
      {
        return -1;
      }
    };
    int r=this->read();
  }
  return 0;
}


int Sim900::sendATCmnd(char *atCmnd, int isEchoOn)
/**
 * \brief     Emission d'une commande AT vers le SIM900
 * \details   "atCmnd" doit contenir la commande sans le préfixe "AT" et sans le CR ou CR/LF. Ces éléments seront
 *            ajoutés par la commande avant l'émission.
 *            Ex : sim900.sendATCmnd("E0",0) ~ sim900.sendString("ATE0\r",0)
 *            Pour récupérer le resultat de la commande utiliser waitString ou équivalant.
 * \param     atCmnd    commande à transmettre
 * \param     isEchoOn  1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  if(sendChar('A', isEchoOn)<0)
    return -1;
  if(sendChar('T', isEchoOn)<0)
    return -1;
  if(sendString(atCmnd, isEchoOn)<0)
    return -1;
  if(sendCr(isEchoOn)<0)
    return -1;

  return 0;
}


int Sim900::sendString(char *str, int isEchoOn)
/**
 * \brief     Transmission d'une chaine de caractères vers le SIM900
 * \details   /!\ n'envoie pas le '\r', il doit être contenu dans la chaine.
 *            nota : pour le SIM900 '\n' est suffisant pour le changement de ligne
 * \param     str       chaine de caractères à transmettre
 * \param     isEchoOn  1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  char c;

  for(int i=0;str[i];i++)
  {
    if(sendChar(str[i],isEchoOn)<0)
      return -1;
  }
  return 0;
}


int Sim900::sendStringFromProgmem(prog_char *str, int isEchoOn)
/**
 * \brief     Transmission d'une chaine de caractères contenue en mémoire Flash(programme) vers le SIM900
 * \details   /!\ n'envoie pas automatiquement le CR/LF ('\r\n'), il doit être contenu dans la chaine.
 *            nota : pour le SIM900 '\n' est suffisant pour le changement de ligne
 * \param     str       chaine de caractères à transmettre
 * \param     isEchoOn  1 si un echo du caractère est attendu (et à ne pas traiter) 0 sinon
 * \return    -1 en cas de timeout "echoTimeout", 0 sinon. /!\ la commande peut ne pas être allée jusqu'au bout.
 *            Utiliser sim900.sync si nécessaire.
 */
{
  int i=0;
  while(1)
  {
    char c =  pgm_read_byte_near(str + i);
    if(c)
      if(sendChar(c,isEchoOn)<0)
        return -1;
      else
        break;
  }
  return 0;
}


unsigned char *Sim900::readLine(long timeout)
/**
 * \brief     lit une ligne en provenance du SIM900.
 * \details   Tous les caractères en provenance du SIM900 sont récupérés jusqu'à trouver un CR/LF.
 *            La fonction retourne alors un pointeur sur la chaine de caractères contenant cette ligne sans le CR/LF.
 *            Si "timeout" ms se sont écoulés avant d'obtenir un CR/LF la lecture s'arrête (la fonction retour NULL dans ce cas).
 * \param     timeout  durée maximum en ms pour obtenir une chaine de caractères terminée par CR/LF
 * \return    pointeur sur le buffer contenant la chaine ou NULL. /!\ le buffer sera réutilisé lors du prochain
 *            appel à readLine.
 *            NULL est retourné en cas de timeout ou si la taille max (SIM900_BUFFER_SIZE) du buffer est atteinte
 *            avant l'arrivée d'un CR/LF.
 */
{
  int c;
  unsigned long start = millis(); // démarrage du chrono
  bufferPtr = 0;

  while(1)
  {
    unsigned long now = millis();
    if(this->available())
    {
      c = (unsigned char)this->read();
      if(c == 10) // fin de ligne trouvée
      {
        buffer[bufferPtr]=0;
        return buffer;
      }
      else if(c == 13)
      {
      }
      else
      {
        buffer[bufferPtr]=c;
        bufferPtr++;
        if(bufferPtr > SIM900_BUFFER_SIZE)
        {
          bufferPtr = 0;
          return NULL; // erreur : overflow
        }
      }
    }

    if(diffMillis(start,  now) > timeout)
    {
      bufferPtr = 0;
      return NULL;
    }
  }
}


int Sim900::waitLines(char *vals[], long timeout)
/**
 * \brief     Attent qu'une ligne de la liste des lignes (tableau de chaines "vals") ait été lue sur le SIM900 et retourne le numéro de ligne trouvée.
 * \details   Cette fonction prend en paramètre un tableau de chaines dont le dernier élément doit être NULL. Les chaines seront
 *            comparées aux lignes lues sur le SIM900.
 *            Dès qu'une ligne (sans le CR/LF) est exactement identique à l'une des chaines de la liste, le numéro de la chaine
 *            trouvée est retourné. La numérotation des lignes commence à 0.
 *            Si "timeout" ms se sont écoulées avant d'obtenir l'une des lignes attendues, la lecture s'arrête (la fonction retour -1 dans ce cas).
 * \param     timeout : durée maximum en ms pour obtenir l'une chaine de caractères recherchée
 * \return    numéro de la ligne (premier élément == 0).
 *            -1 timeout avant de trouver la ligne
 */
{
  unsigned long start = millis(); // démarrage du chrono
  while(1)
  {
    unsigned long now = millis();
    long rl_timeout = timeout - diffMillis(start,  now);
    if(rl_timeout < 0)
    {
      return -1;
    }

    char *l=(char *)readLine(rl_timeout);
    if(l==NULL)
    {
      return -1;
    }
    for(int i=0;vals[i];i++)
    {
      if(strcmp(l,vals[i])==0)
      {
        return (i);
      }
    }
  }
}


int Sim900::waitString(char *val, long timeout)
/**
 * \brief     Attent que la chaine "val" ait été lue sur le SIM900.
 * \details   lit tous les caractères arrivant du SIM900 et dès que la séquence de caractères est strictement identique à "val"
 *            la fonction retourne 0.
 *            Si "timeout" ms se sont écoulées avant la séquence de caractères attendu, la lecture s'arrête (la fonction retour -1 dans ce cas).
 *            Cette fonction n'interprête pas CR/LF. Il est donc possible de faire sim900.waitString("\r\n",100); pour attendre une fin de ligne.
 * \param     timeout  durée maximum en ms pour obtenir la séquence de caractères recherchée
 * \return    0 lorsque la chaine est trouvée,
 *            -1 si "timeout" est atteint avant de trouver la chaine.
 */
{
  unsigned long start = millis(); // démarrage du chrono
  int i=0;

  if(!val[0])
    return 0;
  while(1)
  {
    unsigned long now = millis();

    if(diffMillis(start,  now) > timeout)
    {
      return -1;
    }

    if(this->available())
    {
      int c = (unsigned char)this->read();
      if(c == val[i])
        i++;
      else
      {
        i=0;
        if(c == val[i])
          i++;
      }

      if(!val[i])
      {
        return 0;
      }
    }
  }
}


int Sim900::readStringTo(char *str, int l_str, char *val, long timeout)
/**
 * \brief     récupère tous les caractères en provenance du SIM900 dans "str" jusqu'à ce que la séquence de caractères "val" soit trouvée.
 * \details   "l_str" précise la taille maximum possible pour "str". Si la taille max ('\0' de fin de chaine compris) est atteinte avant 
 *            de trouver la séquence, la fonction retourne -1. str contient cependant tous les caractères déjà lus.
 *            Si "timeout" ms se sont écoulées avant la lecture de la séquence de caractères attendu, la lecture s'arrête,
 *            la fonction retour -1 et str[0] contient '\0';
 *            tous les caractères attendus de "val" sont présents en fin de chaine.
 *            Cette fonction n'interprête pas CR/LF. Il est donc possible de faire sim900.readStringTo("\r\n",100); pour lire une ligne complète.
 * \param     str      contient la chaine trouvée
 * \param     l_str    taille maximum de la chaine (0 de fin de chaine compris)
 * \param     val      séquence de caractères attendue
 * \param     timeout  durée maximum en ms pour obtenir la séquence de caractères recherchée
 * \return    0 lorsque la chaine est trouvée, -1 si "timeout" atteint avant de trouver la chaine (dans ce cas str[0]==0) 
 *            ou si taille max atteinte (avec donc str[l_str-1]==0);
 */
{
  unsigned long start = millis(); // démarrage du chrono
  int i=0;
  int strptr=0;
  while(1)
  {
    unsigned long now = millis();

    if(diffMillis(start,  now) > timeout)
    {
      str[0]=0;
      return -1;
    }

    if(this->available())
    {
      if(strptr >= l_str-1)
      {
        str[l_str-1]=0;
        return -1;
      }

      int c = (unsigned char)this->read();
      str[strptr]=c;
      strptr++;

      if(c == val[i])
        i++;
      else
      {
        i=0; // réinitialisation de la séquence
        if(c == val[0]) // mais ça peut être le début d'une nouvelle séquence
          i++;
      }

      if(!val[i])
      {
        return 0;
      }
    }
  }
}


int Sim900::init()
/**
 * \brief     initialise le SIM900 pour recevoir des SMS texte et supprime l'echo
 * \return    toujours 0.
 */
{
  // configuration du sim900 pour recevoir des SMS
  echoOff();
  //  sendATCmnd("EO",0); // suppression de l'echo
  //  waitLines((char **)standard_returns, 200);

  if(pinCode[0])
  {
    sendString("AT+CPIN=");
    sendString((char *)pinCode);
    sendCr();
    //     waitLines(ok_str, error_str, 200);
    waitLines((char **)SIM900_standard_returns, atTimeout);
  }

  sendATCmnd("+CMGF=1"); // mode text pour les sms
  //  waitLines(ok_str, error_str, 200);
  waitLines((char **)SIM900_standard_returns, atTimeout);

  // format UCS2 pour les données reçu
  // sendATCmnd("+CSCS=\"UCS2\"");
  // waitLines((char **)SIM900_standard_returns, atTimeout);

  sendATCmnd("+CNMI=2,2,0,0,0");
  //  waitLines(ok_str, error_str, 200);
  waitLines((char **)SIM900_standard_returns, atTimeout);

  sendATCmnd("+CSDH=0"); // pour avoir à la reception d'un : +CMT: "+12223334444","","14/05/30,00:13:34-32"
  //  waitLines(ok_str, error_str, 200);
  waitLines((char **)SIM900_standard_returns, atTimeout);

  return 0;
}


int Sim900::sync(long timeout)
/**
 * \brief     synchronise la communication avec un SIM900
 * \details   vide le buffer d'entrée (lecture et oubli jusqu'à plus rien à lire.
 *            envoie une commande AT et attend en retour un OK. Cela permet à l' "autobaud" de choisir la bonne vitesse de communication.
 *            si le retour n'est pas "OK", un nouveau cycle est réalisé.
 *            la tentative de synchro s'arrête après "timeout" ms.
 * \param     timeout  durée maximum accordée à la synchronisation
 * \return    0 si la synchronisation est réussie.
 *            -1 si timeout atteint.
 */
{
  unsigned long start = millis(); // démarrage du chrono

  // attente disponibilité du sim900
  while(1)
  {
    unsigned long now = millis();

    // on vide le buffer d'entrée
    for(;this->available();)
    {
      char c = this->read();
    }

    // Commande AT de synchro
    sendATCmnd(""); // ~ sendString("AT\r");
    
    int ret = waitLines((char **)SIM900_standard_returns, 500);
    if(ret==0)
    {
      return 0;
    }
    
    if(diffMillis(start,  now) > timeout)
      return -1;

    delay(500);
  }
}


int Sim900::run(unsigned char car)
/**
 * \brief     transmet un charactère à traiter par la librairie SIM900.
 * \details   cette fonction permet un traitement asynchrone des données reçues sur la ligne serie du SIM900.
 *            exemple d'utilisation dans la boucle loop :
 *
 *            Sim900 sim900;
 *            void setup()
 *            {
 *               // ne pas oublier de déclarer les fonctions d'entrée/sortie à SIM900 voir setAvailable, setRead, setWrite et setFlush
 *            }
 *
 *            void loop()
 *            {
 *               if(sim900.available())
 *               {
 *                  int c=sim900.read();
 *                  sim900.run((unsigned char)c);
 *               }
 *            }
 *            
 *            dès qu'une sequence CR/LF est détectée, une analyse des données est déclenchée.
 *            si le nombre de données reçues avant la reception d'un séquence CR/LF et supérieur à la
 *            taille du buffer d'entrée la fonction retourne -1 et le buffer est vidé.
 * \param     car : caractère à traiter.
 * \return    -1 aucun traitement n'a été déclenché, 0 sinon.
 */
{
  char c=toupper(car);

  if(c == 10) // fin de ligne, on la traite
  {
    buffer[bufferPtr]=0;
    analyseBuffer();
    bufferPtr=0; // RAZ du buffer
  }
  else if(c == 13) // on n'a rien à faire de CR
  {
  }
  else // on range la donnée dans le buffer de ligne
  {
    buffer[bufferPtr]=c;
    bufferPtr++;
    if(bufferPtr > SIM900_BUFFER_SIZE) // arg, plus de place dans le buffer ...
    {
      bufferPtr = 0;
      return -1;
    }
  }
  return 0;
}


int Sim900::analyseBuffer()
/**
 * \brief     analyse les données d'une ligne obtenue par sim900.run() et déclenche les actions nécessaires.
 * \details   Les actions qui sont (éventuellement) déclenchées sont des callbacks déclarés par :
 *            - void setSMSCallBack(int (*f)(char *, void *)
 *            - void setDefaultCallBack(int (*f)(char *, void *)
 *            Avec f :
 *               une fonction de prototype int f(char *line, void *userData)
 *            line est une sequence de caractère collecté jusqu'à l'apparition d'une sequence CR/LF
 *            userData est une zone de données "utilisateur" déclarée à Sim900 par :
 *               void setUserData(void *data)
 *            Le premier callback est déclenché lorsqu'un SMS arrive
 *            Le second est déclenché pour tout autre "ligne" reçue
 * \return    -1 si la taille max du buffer est atteinte, 0 sinon.
 */
{
  if(smsFlag == 1)
  {
    int ret = 0;
    if(SMSCallBack)
      ret = SMSCallBack((char *)buffer, userData);
    smsFlag == 0;
    return ret;
  }
  else
    lastSMSPhoneNumber[0]=0;

  smsFlag = 0;

  if(strncmp((char *)buffer,"+CMT:",5) == 0)
  {
    // reception d'un SMS

    // traiter ici les autres info de "+CMT:" si nécessaire (ex : récupération de l'expéditeur
    // récupérer ici le numéro de l'émetteur du SMS et le stocker ... à mettre dans "sim900_sms_phone_number".
    // exemple de ligne "+CMT:" à décoder :
    // +CMT: "+12223334444","","14/05/30,00:13:34-32"
    // <oa> = "+12223334444"
    // = ""
    // <scts> = "14/05/30,00:13:34-32"

    int ptr=7; // pointeur juste après le premier "
    int i=0;
    while(buffer[ptr]!='"' && i<19)
    {
      lastSMSPhoneNumber[i]=buffer[ptr];
      i++;
      ptr++;
    }
    lastSMSPhoneNumber[i]=0;
#if __DEBUG__ > 0
    printDebugStringFromProgmem(lastSMS);
    //    Serial.print("LAST SMS: ");
    Serial.println((char *)lastSMSPhoneNumber);
#endif
    smsFlag = 1; // la prochaine ligne est un SMS entrant
    return 0;
  }

  if(defaultCallBack)
  {
#if __DEBUG__ > 0
    printDebugStringFromProgmem(unsolicitedMsg);
    //    Serial.println("CALL UNSOLICITED_MSG CALLBACK");
#endif
    return this->defaultCallBack((char *)buffer,userData);
  }

  // On à rien fait
  return -1;
}


/******************************************************************************/
/******************************************************************************/

// Entrées/Sorties Réservées
#define PIN_SIM900_RESET 3
#define PIN_SIM900_ENABLE 10
#define PIN_MCU_CMD_ONLY 11
#define PIN_MCU_BYPASS_PROCESSING 12


// adresse de base des données en EEPROM
#define EEPROM_ADDR_PIN 0
#define EEPROM_ADDR_NUM 10


// taille max des données en EEPROM
#define NUMSIZE 21 // taille d'un numero de telephone (20 caractères maximum)
#define PINCODESIZE 9 // taille d'un code pin (8 caractères maximum)


// Variables globales :
// Buffer des résultats
#define CMNDRESULTSBUFFERSIZE 141
unsigned char cmndResultsBuffer[CMNDRESULTSBUFFERSIZE]; // zone de donnée du buffer
int cmndResultsBufferPtr; // pointeur arrivée de caractères dans le buffer
// Buffer d'entrées pour les traitement MCU
#define MCUSERIALBUFFERSIZE 200
int mcuSerialBufferPtr=0; // pointeur arrivée de caractères dans le buffer
unsigned char mcuSerialBuffer[MCUSERIALBUFFERSIZE];

// structure "userData" (pour les callback et les fonctions de traitement)
struct data_s
{
  char dest;
  unsigned char *cmndResultsBuffer;
  int *cmndResultsBufferPtr;
  unsigned char *lastSMSPhoneNumber;
} 
sim900UserData, mcuUserData; // deux zones de données utilisateurs.


// objets globaux
AltSoftSerial sim900Serial;
Sim900 sim900;
BlinkLeds myBlinkLeds(500);


#define  PINSWATCHER_NBPINS 2
struct pinsWatcherData_s
{
  unsigned char pin;
  unsigned long lastChrono;
  int lastState;
} 

pinsWatcherData[PINSWATCHER_NBPINS] = {
  {
    4,0L,-1      }
  ,{
    7,0L,-1      }
};


int setPin(char *num)
/**
 * \brief     copie le "code pin" d'une carte SIM en EEPROM.
 * \param
 * \return    -1 si taille de "num" >= PINCODESIZE, 0 sinon;
 */
{
  int i;
  if(strlen(num)>=PINCODESIZE)
    return -1;
  for(i=0;num[i] && i<(PINCODESIZE-1);i++)
    EEPROM.write(i+EEPROM_ADDR_PIN,num[i]);
  EEPROM.write(i+EEPROM_ADDR_PIN,0);
  return 0;
}


int getPin(char *pin, int l_pin)
/**
 * \brief     récupère dans une chaine le code pin stocké en EEPROM
 * \details   récupère au moins les "l_pin" premier caractères. Prévoir "l_pin" >= à PINCODESIZE
 *            pour récupérer le code en entier.
 * \return    -1 si aucun code n'est positionné, 0 sinon;
 */
{
  int i;
  char c=EEPROM.read(EEPROM_ADDR_PIN);

  if(c==0)
    return -1; // pas de code PIN positionné
  else
    pin[0]=c;

  for(i=1;i<(PINCODESIZE-1) && i<(l_pin-1);i++)
    pin[i]=EEPROM.read(i+EEPROM_ADDR_PIN);
  pin[i]=0;
  return 0;
}


int clearPin()
/**
 * \brief     efface le code pin de l'EEPROM
 * \return    toujours 0;
 */
{
  for(int i=0;i<PINCODESIZE;i++)
    EEPROM.write(i+EEPROM_ADDR_PIN,0);
  return 0;
}


int setNum(int pos, char *num)
/**
 * \brief     ajoute un numéro de téléphone à la position "pos" du carnet
 * de numéro de téléphone stocké en EEPROM.
 * \param     pos   position dans le carnet du numéro de tel (0 à 9)
 * \param     num   chaine de caractères contenant le numéro à écrire
 * \return    -1 si taille de "num" >= NUMSIZE ou "pos" incorrect, 0 sinon;
 */
{
  int i;
  if(pos<0 && pos > 9)
    return -1;
  if(strlen(num)>=NUMSIZE)
    return -1;
  int base=EEPROM_ADDR_NUM+pos*NUMSIZE;
  for(i=0;num[i] && i<(NUMSIZE-1);i++)
    EEPROM.write(base++,num[i]);
  EEPROM.write(base,0);
  return 0;
}


int getNum(int pos, char *num, int l_num)
/**
 * \brief     récupère dans une chaine le numéro de telephone de la position "pos"
 *            stocké en EEPROM
 * \details   récupère au moins les "l_num" premiers caractères. Prévoir "l_num" >= à NUMSIZE
 *            pour récupérer le numéro en entier.
 * \param     pos   position dans le carnet du numéro de tel (0 à 9)
 * \param     num   chaine de caractères contenant le numéro lu
 * \param     l_num nombre maximum de caractères de "num" (0 de fin compris)
 * \return    -1 erreur de position, 0 sinon;
 */
{
  int i;
  if(pos<0 && pos > 9)
    return -1;
  int base=EEPROM_ADDR_NUM+pos*NUMSIZE;
  for(i=0;i<(NUMSIZE-1) && i<(l_num-1);i++)
    num[i]=EEPROM.read(base++);
  num[i]=0;
  return 0;
}


int numExist(char *num)
/**
 * \brief     controle l'existance d'un numéro dans le carnet de numéro de téléphone
 *            stocké en EEPROM
 * \param     num   chaine de caractères contenant le numéro à controler
 * \param     l_num nombre maximum de caractères de "num" (0 de fin compris)
 * \return    0 si le numéro est présent, -1 sinon;
 */
{
  if(num[0]==0) // num ne peut pas être "rien"
    return -1;

  for(int i=0;i<10;i++)
  {
    int flag=0;
    for(int j=0;j<NUMSIZE;j++)
    {
      char c=EEPROM.read(j+EEPROM_ADDR_NUM);
      if(c != num[j])
        break;
      else if(c==0 && num[j]==0)
      {
        flag=1;
        break;
      }
      else if(c==0 || num[j]==0)
        break;
    }
    if(flag==1)
      return 0;
  }
  return -1;
}


int listRom()
/**
 * \brief     affiche le contenu de l'EEPROM
 * \return    toujours 0;
 */
{
  Serial.print("PIN=");
  for(int i=0;i<PINCODESIZE;i++)
  {
    char c=EEPROM.read(i+EEPROM_ADDR_PIN);
    if(c==0)
    {
      Serial.write('\n');
      break;
    }
    else
      Serial.write(c);
  }

  for(int i=0;i<10;i++)
  {
    Serial.print("NUM");
    Serial.print(i);
    Serial.write('=');
    for(int j=0;j<NUMSIZE;j++)
    {
      char c=EEPROM.read(j+EEPROM_ADDR_NUM);
      if(c==0)
      {
        Serial.write('\n');
        break;
      }
      else
        Serial.write(c);
    }
  }
  return 0;
}


int addToCmndResults(struct data_s *data, int pin, int cmnd, long value)
/**
 * \brief     ajoute le résultat d'une commande dans le buffer des résultats d'execution
 * \param     data  données spécifiques au traitement (user data de callback)
 * \param     pin   numéro de la ligne
 * \param     cmnd  la commande
 * \param     value le résultat de la commande
 * \return    -1 le buffer ne peut pas contenir le resultat, 0 sinon
 */
{
  // à mettre dans cmndResults sous la forme : PINx=VALUE1;PINx=VALUE2 ... max 140 caractères
  char tmpstr[10];

  itoa(value,tmpstr,10);

  if(*data->cmndResultsBufferPtr<CMNDRESULTSBUFFERSIZE-strlen(tmpstr)-8)
  {
    if(*data->cmndResultsBufferPtr!=0)
      data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=';';

    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=cmnd;
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='/';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='P';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='I';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='N';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=(char )(pin+'0');
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='=';
    for(int i=0;tmpstr[i];i++)
      data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=tmpstr[i];
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr]=0;
    return 0;
  }
  return -1;
}


int doneToCmndResult(struct data_s *data)
/**
 * \brief     ajoute "DONE" dans le buffer des résultats d'execution
 * \param     data  données spécifiques au traitement (user data de callback)
 * \return    -1 le buffer ne peut pas contenir le resultat, 0 sinon
 */
{
  if(*data->cmndResultsBufferPtr<CMNDRESULTSBUFFERSIZE-5)
  {
    if(*data->cmndResultsBufferPtr!=0)
      data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=';';

    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='D';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='O';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='N';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='E';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr]=0;

    return 0;
  }
  return -1;
}


int errToCmndResult(struct data_s *data, int errno)
/**
 * \brief     ajoute une erreur dans le buffer des résultats d'execution
 * \param     data    données spécifiques au traitement (user data de callback)
 * \param     errno   numéro d'erreur (0 = erreur 'A', 1 = erreur 'B', 26 = erreur 'Z'
 * \return    -1 le buffer ne peut pas contenir le resultat, 0 sinon
 */
{
  if(*data->cmndResultsBufferPtr<CMNDRESULTSBUFFERSIZE-6)
  {
    if(*data->cmndResultsBufferPtr!=0)
      data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=';';

    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='E';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='R';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]='R';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=':';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr++]=errno+'A';
    data->cmndResultsBuffer[*data->cmndResultsBufferPtr]=0;

    return 0;
  }
  return -1;
}


int sendCmndResults(struct data_s *data)
/**
 * \brief     emet le resultat d'execution vers la ligne Serie ou sous forme d'un SMS.
 * \param     data    données spécifiques au traitement (user data de callback)
 * \return    toujours 0
 */
{
  if(data->dest==1)
  {
    if(sim900.sync(5000)==0)
      sim900.sendSMS((char *)data->lastSMSPhoneNumber, (char *)data->cmndResultsBuffer);
  }
  else
  {
    Serial.println((char *)data->cmndResultsBuffer);
  }
}


int doCmnd(struct data_s *data, int pin, int cmnd, long value)
/**
 * \brief     execute la commande spécifié (si conforme)
 * \param     data    données spécifiques au traitement (user data de callback)
 * \param     pin   numéro de la ligne
 * \param     cmnd  code de la commande
 * \param     value parametre de la commande
 * \return    -1 la commande n'a pas été exécuté, 0 sinon
 */
{
  static char pins[] = {
    4,5,6,7,14,15,16,17,18,19 // pin 4 et 5 en "read-only"
  };

  int v;

  if(pin<0 || pin>9)
  {
    errToCmndResult(data, 11);
    return -1;
  }

  if(value < -1 || value > 0xFFFF)
  {
    errToCmndResult(data, 11);
    return -1;
  }

  switch(cmnd)
  {
  case 'L':
    if(value==-1)
    {  // lecture état logique
      if(pin!=0 & pin != 3)
        pinMode(pins[pin],INPUT);
      v=digitalRead(pins[pin]);
      addToCmndResults(data,pin,cmnd,v);
    }
    else
    {  // positionne état logique sortie
      if(pin==0 || pin == 1) // lecture seule
      {
        errToCmndResult(data, 12);
        return -1;
      }
      pinMode(pins[pin],OUTPUT);
      digitalWrite(pins[pin],(int)value);
    }
    doneToCmndResult(data);
    return 0;
  case 'A':
    if(value==-1 && pins[pin] >= 14)
    {
      v=analogRead(pins[pin]-14);
      addToCmndResults(data,pin,cmnd,v);
      return 0;
    }
    else if(value!=-1 && (pin ==1 || pin == 2))
    {
      pinMode(pins[pin],OUTPUT);
      analogWrite(pins[pin],value & 0x3FF);
      doneToCmndResult(data);
      return 0;
    }
    else
    {
      errToCmndResult(data, 13);
      return -1;
    }
  default:
    errToCmndResult(data, 14);
    return -1;
  }
}


int evalCmndString(char *buffer, void *dataPtr)
/**
 * \brief     décode une ligne de commandes et déclenche les traitements associés.
 * \details   le format de la ligne de commandes est le suivant :
 * ##{PIN1},{COMMANDE1},{PARAM1};{PIN2},{COMMANDE2},{PARAM2};...## (aucun espace accepté).
 * avec :
 * PIN       numéro de la sortie/sortie Interface_type_003
 * correspondance avec I/O Arduino (Pin Arduino/Interface_type_003) : 0:4, 1:5, 2:6, 3:7, 4:14(A0), 5:15(A1), 6:16(A2), 7:17(A3), 8:18(A4), 9:19(A5)
 * COMMANDE  action à réaliser :
 * - 'L' : nouvelle valeur logique
 * - 'A' : nouvelle valeur analogique
 * PARAM   parametre de la commande :
 * - 'L' => H (set high), L (set low)
 * - 'A' => entier positif
 * - pour 'L' et 'A' => 'G' = Get value : lecture d'une valeur
 * exemple : ##0,L,H;1,A,10;4:A:G##
 * PIN 0 positionnée à HIGH
 * PIN 1 PWM à 10
 * lecture PIN 4
 * Le résultat de la commande sera :
 * DONE;DONE;PIN4=234
 * \param     buffer    pointeur sur la ligne de commande à analyser
 * \param     dataPtr   données spécifiques au traitement (user data de callback)
 * \return    -1 une erreur c'est produite, le traitement n'a pas été jusqu'au bout mais certaine
 *            commande on pu être passé. 0 sinon
 */
{
  int retour=-1;
  int l=strlen((char *)buffer); // nombre de caractères (encore à traiter)
  struct data_s *data = (struct data_s *)dataPtr;

  if(data->dest==1 && data->lastSMSPhoneNumber[0]==0)
  {
    errToCmndResult(data, 1);
    goto evalCmndString_clean_exit;
  }

  if(data->dest==1 && numExist((char *)data->lastSMSPhoneNumber)==-1)
  {
    errToCmndResult(data, 2);
    goto evalCmndString_clean_exit;
  }

  data->cmndResultsBufferPtr=0;

  if(buffer[0]  =='#' &&
    buffer[1]  =='#' &&
    buffer[l-2]=='#' &&
    buffer[l-1]=='#') // une commande via SMS ?
  {
    int ptr=2;
    l=l-4; // 4 "#" déjà traité

    while(1)
    {
      int pin = 0;
      char cmnd = 0;
      long valeur = 0;

      if(!l)
      {
        errToCmndResult(data, 3);
        goto evalCmndString_clean_exit;
      }

      // lecture d'un numéro de pin : 1 ou 2 caractères numériques attendus
      if(buffer[ptr]>='0' && buffer[ptr]<='9') // premier caractère numérique ?
      {
        pin=buffer[ptr]-'0';
      }
      else
      {
        errToCmndResult(data, 4);
        goto evalCmndString_clean_exit;
      }

      ptr++; // passage au caractère suivant
      l--;
      if(!l)
      {
        errToCmndResult(data, 5);
        goto evalCmndString_clean_exit;
      }

      if(buffer[ptr]>='0' && buffer[ptr]<='9') // deuxième caractère numérique ?
      {
        pin=pin *10 + buffer[ptr]-'0';
        ptr++;
        l--;
      }
      else // si deuxième caractère non numérique on doit avoir un ';', traité par la lecture du séparateur
      {
      }

      // lecture du séparateur
      if(!l || buffer[ptr]!=',')
      {
        errToCmndResult(data, 6);
        goto evalCmndString_clean_exit;
      }
      ptr++;
      l--;
      if(!l)
      {
        errToCmndResult(data, 7);
        goto evalCmndString_clean_exit;
      }

      // lecture de la commande
      switch(buffer[ptr])
      {
      case 'L': // sortie logique
      case 'A': // sortie analogique
        cmnd = buffer[ptr];
        break;
      default:
        errToCmndResult(data, 8);
        goto evalCmndString_clean_exit;
      }
      ptr++;
      l--;

      // lecture du séparateur
      if(!l || buffer[ptr]!=',')
        goto evalCmndString_clean_exit;
      ptr++;
      l--;
      if(!l)
      {
        errToCmndResult(data, 9);
        goto evalCmndString_clean_exit;
      }

      // lecture parametre
      if(buffer[ptr]=='H' || buffer[ptr]=='L')
      {
        if(buffer[ptr]=='H')
          valeur=1;
        else
          valeur=0;
        ptr++;
        l--;
      }
      // lecture 'G'
      else if(buffer[ptr]=='G')
      {
        valeur=-1;
        ptr++;
        l--;
      }
      else
        // lecture valeur numérique
      {
        while(1)
        {
          if(l && buffer[ptr]>='0' && buffer[ptr]<='9')
          {
            valeur=valeur *10 + buffer[ptr]-'0';
            ptr++;
            l--;
          }
          else if(!l || buffer[ptr]==';')
          {
            break; // point de sortie de la boucle
          }
          else
          {
            errToCmndResult(data, 10);
            goto evalCmndString_clean_exit;
          }
        }
      }

      // à partir d'ici on traiter la demande
      doCmnd(data, pin, cmnd, valeur);
      // fin de traitement de la demande

      if(l)
      {
        ptr++;
        l--;
      }
      else
        break;
    }
  }

  retour=0;

evalCmndString_clean_exit:
  sendCmndResults(data);
  return retour;
}


int sim900_read()
/**
 * \brief     wrapping de read pour la fonction de lecture de Sim900.
 * \param     car    caractère à écrire
 * \return    toujours 0
 */
{
  int c=sim900Serial.read();

  Serial.print("[");
  if(c<' ')
     Serial.print(c);
  else
     Serial.print((char)c);
  Serial.print("]");

  return c;
}


int sim900_write(char car)
/**
 * \brief     wrapping de write pour la fonction d'écriture de Sim900.
 * \param     car    caractère à écrire
 * \return    toujours 0
 */
{
  sim900Serial.write(car);
  return 0;
}


int sim900_available()
{
  /**
   * \brief     wrapping "nb caractère disponible" pour Sim900.
   * \return    nombre de caractères disponibles dans le buffer
   */
  return sim900Serial.available();
}


int sim900_flush()
/**
 * \brief     wrapping "flush" pour Sim900.
 * \return    toujours 0
 */
{
  sim900Serial.flush();
  return 0;
}


inline unsigned long diffMillis(unsigned long chrono, unsigned long now)
/**
 * \brief     différence entre chrono et now (now - chrono) en tenant compte
 *            du repassage éventuel par 0.
 * \return    resultat de now - chrono
 */
{
  return now >= chrono ? now - chrono : 0xFFFFFFFF - chrono + now;
}


void sim900_broadcastSMS(char *text)
/**
 * \brief     envoie 1 message à tous les numéros du carnet d'adresse
 * \param     text   message à envoyer.           
 * \return    resultat de now - chrono
 */
{
  char num[NUMSIZE];
  for(int i=0;i<10;i++)
  {
    for(int j=0;j<NUMSIZE;j++)
    {
      char c=EEPROM.read(j+EEPROM_ADDR_NUM);
      if(c==0)
      {
        num[j]=0;
        break;
      }
    }
    if(num[0])
    {
      Serial.print(num);
      Serial.print(':');
      Serial.println(text);

      sim900.sendSMS(num,text);
    }
  }
}


void pinsWatcher()
/**
 * \brief     surveille les changements d'état des lignes "POWER" et "ALARM" et réagit en concéquence.
 * \details   la surveillance gère un "anti-rebond" sur les deux lignes.
 *            dès qu'un frond (montant ou décendant) est validées un message SMS est envoyé.         
 */
{
  //  unsigned char pins[PINSWATCHER_NBPINS]={4,7};
  //  static unsigned long lastChrono[PINSWATCHER_NBPINS]={0,0};
  //  static int lastState[PINSWATCHER_NBPINS]={-1,-1};

  for(int i=0;i<PINSWATCHER_NBPINS;i++)
  {
    int s=digitalRead(pinsWatcherData[i].pin); // lecture de l'entrée
    if(pinsWatcherData[i].lastState != s)
    {
      if(pinsWatcherData[i].lastState==-1) // premier passage, on initialise
      {
        pinsWatcherData[i].lastState=s;
        break;
      }

      unsigned long now=millis();
      if(now == 0)
        now=1; // on perd 1 ms si on tombe pile poil sur 0 ... j'ai trop besoin du 0 ...

      if(pinsWatcherData[i].lastChrono==0) // front detecté
        pinsWatcherData[i].lastChrono=now; // on démarre le chrono
      else if(diffMillis(pinsWatcherData[i].lastChrono,now) > 20) // plus de 20 ms depuis le front ?
      { //oui : on prend en compte
        pinsWatcherData[i].lastState=s;
        pinsWatcherData[i].lastChrono=0;
        // faire ici ce qu'il y a a faire avec ce nouvel etat
        // à revoir pour rendre plus asynchrone ...
        switch(i)
        {
        case 4:
          if(s==LOW) // front descendant
          {
            sim900_broadcastSMS("POWERDOWN");
          }
          else // front montant
          {
            sim900_broadcastSMS("POWERUP");
          }
          break;
        case 7:
          if(s==LOW)
          {
            sim900_broadcastSMS("ALARMEON");
          }
          else
          {
            sim900_broadcastSMS("ALARMEOFF");
          }
          break;
        };
        // fin partie à revoir
      }
    }
    else
      pinsWatcherData[i].lastChrono=0;
  }
}


#define MCU_DONE      0
#define MCU_ERROR     1
#define MCU_ROMERROR  2
#define MCU_CMNDERROR 3
#define MCU_PROMPT    4
#define MCU_OVERFLOW  5

prog_char mcu_error[]     PROGMEM  = { 
  "SYNTAX ERROR" };
prog_char mcu_done[]      PROGMEM  = { 
  "DONE" };
prog_char mcu_romerror[]  PROGMEM  = { 
  "ROM ERROR" };
prog_char mcu_cmnderror[] PROGMEM  = { 
  "CMND ERROR" };
prog_char mcu_prompt[]    PROGMEM  = { 
  "> " };
prog_char mcu_overflow[]  PROGMEM  = { 
  "OVERFLOW" };

int mcuError(int errno)
{
  prog_char *str;
  switch(errno)
  {
  case MCU_ERROR:
    str = mcu_error;
    break;
  case MCU_DONE:
    str = mcu_done;
    break;
  case MCU_ROMERROR:
    str = mcu_romerror;
    break;
  case MCU_CMNDERROR:
    str = mcu_cmnderror;
    break;
  case MCU_PROMPT:
    str = mcu_prompt;
    break;
  case MCU_OVERFLOW:
    str = mcu_overflow;
    break;
  default:
    break;
  }

  int i=0;
  while(1)
  {
    char c =  pgm_read_byte_near(str + i);
    if(c)
      Serial.write(c);
    else
    {
      Serial.write('\n');
      break;
    }
  }
  return 0;
}


int analyseMCUCmnd(char *buffer, struct data_s *data)
{
  // les commandes :
  // PIN:1234 - stockage du code pin de la sim en ROM
  // PIN:C - efface le code pin
  // NUM:x,nnnnnnnnnnnnnnnnnnn - ajoute un numero de telephone dans l'annuaire du MCU (10 positions disponibles, de 0 à 9)
  // NUM:x,C - efface un numéro
  // CMD:##...## - voir evalCmndString - demande l'execution de commandes
  // LST - list le contenu de la ROM
  if(buffer[0]==0)
  {
    mcuError(MCU_PROMPT);
    return 0;
  }

  if(strstr((char *)buffer,"PIN:")==(char *)buffer)
  {
    if(buffer[4]=='C' && buffer[5]==0)
    {
      int ret = clearPin();
      if(ret<0)
        mcuError(MCU_ROMERROR);
      else
        mcuError(MCU_DONE);
      return ret;
    }

    char pinCode[9];
    int i;
    for(i=0;i<8 && buffer[i+4];i++)
    {
      if(buffer[i+4] >= '0' && buffer[i+4] <='9')
        pinCode[i]=buffer[i+4];
      else
      {
        mcuError(MCU_ERROR);
        return -1; // syntax error
      }
    }
    pinCode[i]=0;
    // stocker le code en ROM
    setPin(pinCode);
    mcuError(MCU_DONE);
    return 0;
  }

  if(strstr((char *)buffer,"NUM:")==(char *)buffer)
  {
    char num[21];
    int x=buffer[4]-'0';
    if(x >=0 && x<=9)
    { 
      if(buffer[5]==',')
      {
        if(buffer[6]=='C' && buffer[7]==0)
        {
          int ret;
          ret = setNum(x, NULL);
          if(ret<0)
            mcuError(MCU_ROMERROR);
          else
            mcuError(MCU_DONE);
          return ret;
        }

        int i;
        for(i=0;i<(NUMSIZE-1) && buffer[i+6];i++)
          num[i]=buffer[i+6];
        num[i]=0;
        // stocker le numero à la position x en ROM
        int ret=setNum(x, num);
        if(ret<0)
          mcuError(MCU_ROMERROR);
        else
          mcuError(MCU_DONE);
        return ret;
      }
      else
      {
        mcuError(MCU_ERROR);
        return -1;
      }
    }
    else
    {
      mcuError(MCU_ERROR);
      return -1;
    }
  }

  if(strstr((char *)buffer,"CMD:")==(char *)buffer)
  {
    int ret = evalCmndString((char *)&(buffer[4]), (void *)data);
    if(ret < 0)
      mcuError(MCU_CMNDERROR);
    else
      mcuError(MCU_DONE);
    return ret;
  }

  if(strstr((char *)buffer,"LST")==(char *)buffer)
  {
    int ret = listRom();
    if(ret<0)
      mcuError(MCU_ROMERROR);
    else
      mcuError(MCU_DONE);
    return ret;
  }

  mcuError(MCU_ERROR);
  return -1;
}


int processCmndFromSerial(unsigned char car, struct data_s *data)
{
  char c=toupper(car);

  if(c == 13) // fin de ligne, on la traite
  {
    mcuSerialBuffer[mcuSerialBufferPtr]=0;
    if(mcuSerialBufferPtr == 0)
    {
      mcuError(MCU_PROMPT);
    }
    else
    {
      analyseMCUCmnd((char *)mcuSerialBuffer, data);
      mcuSerialBufferPtr=0; // RAZ du buffer
      mcuError(MCU_PROMPT);
    }
  }
  else if(c == 10) // on n'a rien à faire de LF
  {
  }
  else // on range la donnée dans le buffer de ligne
  {
    mcuSerialBuffer[mcuSerialBufferPtr]=(char)c;
    mcuSerialBufferPtr++;
    if(mcuSerialBufferPtr >= MCUSERIALBUFFERSIZE) // arg, plus de place dans le buffer ...
    {
      mcuSerialBufferPtr = 0;
      mcuError(MCU_OVERFLOW);
      return -1;
    }
  }
  return 0;
}


int sim900_connected = 0; // passé à 1 si un sim900 est détecté lors de l'initialisation

void setup()
{
  char *line;

  // initialisation port de communication Serie
  Serial.begin(57600);
  // intialisation port communication avec sim900
  sim900Serial.begin(9600);

  // Reset "hardware" du sim900
  pinMode(PIN_SIM900_RESET,OUTPUT);
  digitalWrite(PIN_SIM900_RESET,HIGH);
  delay(100);
  digitalWrite(PIN_SIM900_RESET,LOW);
  delay(1000);
  digitalWrite(PIN_SIM900_RESET,HIGH);

  // Led d'état, début com. avec SIM900
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  delay(5000); // on laisse le temps au sim900 de s'initialiser
  digitalWrite(13, HIGH);

  Serial.print("Init Arduino ...");
  // déclaration des zones de données pour callback
  // initialisation des données
  mcuUserData.dest=0;
  // buffer partager pour sim900 et MCU
  mcuUserData.cmndResultsBuffer=cmndResultsBuffer;
  mcuUserData.cmndResultsBufferPtr=&cmndResultsBufferPtr;
  // pas de numéro pour MCU
  mcuUserData.lastSMSPhoneNumber=NULL;

  sim900UserData.dest=1;
  // buffer partager pour sim900 et MCU
  sim900UserData.cmndResultsBuffer=cmndResultsBuffer;
  sim900UserData.cmndResultsBufferPtr=&cmndResultsBufferPtr;
  sim900UserData.lastSMSPhoneNumber=sim900.getLastSMSPhoneNumber();

  // association objet sim900 avec le port de com.
  sim900.setRead(sim900_read);
  sim900.setWrite(sim900_write);
  sim900.setAvailable(sim900_available);
  sim900.setFlush(sim900_flush);

  // déclaration des données utilisateur à passer aux callbacks
  sim900.setUserData((void *)&sim900UserData);

  // déclaration des callbacks
  sim900.setSMSCallBack(evalCmndString);

  Serial.print("com. with SIM900 ...");
  // synchronisation avec le sim900
  if(sim900.sync(10000)!=-1) // 10 secondes pour se synchroniser
  {
    // récupération du code PIN si nécessaire
    char pinCode[PINCODESIZE];
    if(getPin(pinCode,PINCODESIZE)==0)
      sim900.setPinCode(pinCode);

    sim900.init(); // préparation "standard" du sim900
    sim900_connected = 1;
  }
  else
    myBlinkLeds.setInterval(125);
  /*
  else
   {
   // faire quelque chose ici si on arrive pas à se synchroniser
   // => refaire reset matériel du SIM900 ?
   BlinkLeds myBlinkLeds_125ms(125);
   while(1) // boucle sans fin avec clignotement rapide
   {
   myBlinkLeds_125ms.run();
   digitalWrite(13, myBlinkLeds_125ms.getLedState()); // clignotement de la led "activité" (D13) de l'ATmega
   }
   }
   */
  Serial.println("done");

  // communication sim900 établie
  digitalWrite(13, LOW); // initialisation terminée

  pinMode(PIN_MCU_CMD_ONLY, INPUT);
  digitalWrite(PIN_MCU_CMD_ONLY, HIGH); // pullup activé
  pinMode(PIN_SIM900_ENABLE, INPUT);
  digitalWrite(PIN_SIM900_ENABLE, HIGH); // pullup activé
  pinMode(PIN_MCU_BYPASS_PROCESSING, INPUT);
  digitalWrite(PIN_MCU_BYPASS_PROCESSING, HIGH); // pullup activé

  // entrée d'alarme
  pinMode(4, INPUT); // detection de tension
  digitalWrite(4, HIGH); // pullup activé
  pinMode(7, INPUT); // déclenchement alarme
  digitalWrite(7, HIGH); // pullup activé
}


void loop()
{
  myBlinkLeds.run();
  digitalWrite(13, myBlinkLeds.getLedState()); // clignotement de la led "activité" (D13) de l'ATmega

  pinsWatcher();

  if(Serial.available())
  {
    char serialInByte;

    serialInByte = (unsigned char)Serial.read();
    if(!sim900_available || digitalRead(PIN_MCU_CMD_ONLY)==LOW)
      processCmndFromSerial(serialInByte, &mcuUserData); // si PIN MCU_CMD_ONLY bas, les données sont destinées au MCU uniquement.
    else
    {
      if(sim900_available)
        sim900.write(serialInByte); // toutes les données de la ligne serie sont envoyées vers le sim900
    }
  }

  if(sim900_available && sim900.available())
  {
    char sim900SerialInByte;

    sim900SerialInByte = (unsigned char)sim900.read();

    if(digitalRead(PIN_SIM900_ENABLE)==HIGH) // les données du sim900 ne sont pas retransmis si LOW
      Serial.write(sim900SerialInByte);

    if(digitalRead(PIN_MCU_BYPASS_PROCESSING)==HIGH) // les données du SIM900 ne sont pas traité localement par le MCU
      sim900.run(sim900SerialInByte);
  }
}

