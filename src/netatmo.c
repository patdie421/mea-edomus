#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "netatmo.h"
#include <curl/curl.h>
#include "cJSON.h"
#include "curl_adds.h"
#include "mea_verbose.h"
#include "mea_string_utils.h"

char *netatmo_therm_mode[]={"program", "away", "hg", "manual", "off", "max", NULL};

static int _netatmo_parse_return_json(char *response, char *err, int l_err)
{
   int ret_code=-1;

   cJSON *response_cjson=cJSON_Parse(response);
   if(response_cjson==NULL)
   {
       DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
       goto _netatmo_parse_return_json_clean_exit;
   }

   cJSON *e=cJSON_GetObjectItem(response_cjson, "status");
   if(e && e->type==cJSON_String)
   {
      if(strcmp(e->valuestring, "ok")==0)
         ret_code=0;
      else
      {
         if(err)
            strncpy(err, e->valuestring, l_err);
         ret_code=1;
      }
   } 

_netatmo_parse_return_json_clean_exit:
   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }
   return ret_code;
}


static int _netatmo_parse_data_json(char *response, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data,  char *err, int l_err)
{
   int ret_code=-1;

   cJSON *response_cjson=cJSON_Parse(response);
   if(response_cjson==NULL)
   {
       DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
       goto _netatmo_parse_data_json_clean_exit;
   }

   cJSON *e = NULL;

   e=cJSON_GetObjectItem(response_cjson, "error");
   if(e)
   {
      cJSON *a=cJSON_GetObjectItem(e, "message");
      if(a)
      {
         if(err)
            strncpy(err, a->valuestring, l_err);
      }
      else
      {
         if(err)
            strncpy(err,"unknown error", l_err);
      }
      a=cJSON_GetObjectItem(e, "code");
      if(a)
         ret_code=a->valueint;
      else
         ret_code=9999;
      goto _netatmo_parse_data_json_clean_exit;
   }

   ret_code=-2;
   e=cJSON_GetObjectItem(response_cjson, "body"); 
   if(e==NULL)
      goto _netatmo_parse_data_json_clean_exit;

   ret_code=-3;
   e=cJSON_GetObjectItem(e, "devices");
   if(e==NULL || e->type!=cJSON_Array)
       goto _netatmo_parse_data_json_clean_exit;

   ret_code=-4;
   e=cJSON_GetArrayItem(e, 0);
   if(e==NULL)
       goto _netatmo_parse_data_json_clean_exit;

   ret_code=-5;
   e=cJSON_GetObjectItem(e, "modules");
   if(e==NULL || e->type!=cJSON_Array)
       goto _netatmo_parse_data_json_clean_exit;
 
   int i=0;
   cJSON *a;
   do
   {
      ret_code=-6;
      a=cJSON_GetArrayItem(e, i); 
      if(a)
      {
         cJSON *e;
         ret_code=-7;
         e=cJSON_GetObjectItem(a, "_id");
         if(e==NULL)
            goto _netatmo_parse_data_json_clean_exit;
         if(strcmp(e->valuestring, thermostat_id)==0)
         {
            ret_code=-8;
            e=cJSON_GetObjectItem(a, "battery_vp");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->battery_vp=e->valueint;

            ret_code=-9;
            e=cJSON_GetObjectItem(a, "therm_relay_cmd");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->therm_relay_cmd=e->valueint;

            ret_code=-10;
            e=cJSON_GetObjectItem(a, "setpoint");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            {
               ret_code=-11;
               char *s=cJSON_GetObjectItem(e, "setpoint_mode")->valuestring;
               if(s==NULL)
                  goto _netatmo_parse_data_json_clean_exit;
               if(strcmp(s,"program")==0)
                  thermostat_data->setpoint=PROGRAM;
               else if(strcmp(s, "away")==0)
                  thermostat_data->setpoint=AWAY;
               else if(strcmp(s,"hg")==0)
                  thermostat_data->setpoint=HG;
               else if(strcmp(s,"manual")==0)
                  thermostat_data->setpoint=MANUAL;
               else if(strcmp(s,"off")==0)
                  thermostat_data->setpoint=OFF;
               else if(strcmp(s,"max")==0)
                  thermostat_data->setpoint=MAX;
               else
                  thermostat_data->setpoint=-1; 
            }

            ret_code=-12;
            cJSON *mesured=cJSON_GetObjectItem(a, "measured");
            if(mesured==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            else
            {
               ret_code=-13;
               e=cJSON_GetObjectItem(mesured, "temperature");
               if(e==NULL)
                  goto _netatmo_parse_data_json_clean_exit;
               thermostat_data->temperature=e->valuedouble;

               ret_code=-14;
               e=cJSON_GetObjectItem(mesured, "setpoint_temp");
               if(e==NULL)
                  goto _netatmo_parse_data_json_clean_exit;
               thermostat_data->setpoint_temp=e->valuedouble;
               ret_code=0;
               break;
            }
         }
         i++;
      }
   }
   while(a);

_netatmo_parse_data_json_clean_exit:
   if(ret_code<0)
   {
      if(err)
         strcpy(err,"parsing data error");
   }

   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }

   return ret_code;
}



static int _netatmo_parse_token_json(char *response, struct netatmo_token_s *token, char *err, int l_err)
{
   int ret_code=-1;
   cJSON *response_cjson=cJSON_Parse(response);

   if(response_cjson==NULL)
   {
       DEBUG_SECTION mea_log_printf("%s (%s) : JSON error\n", DEBUG_STR, __func__);
       goto netatmo_parse_token_json_clean_exit;
   }

   cJSON *e = NULL;

    e=cJSON_GetObjectItem(response_cjson,"error");
    if(e)
    {
       VERBOSE(2) mea_log_printf("%s (%s) : NETATMO error : %s\n", DEBUG_STR, __func__, e->valuestring);
       if(err)
          strncpy(err, e->valuestring, l_err);
       ret_code=1;
       goto netatmo_parse_token_json_clean_exit;
    }

    e=cJSON_GetObjectItem(response_cjson,"access_token");
   if(e && e->type==cJSON_String)
      strcpy(token->access, e->valuestring);
   else goto netatmo_parse_token_json_clean_exit;

   e=cJSON_GetObjectItem(response_cjson,"refresh_token");
   if(e && e->type==cJSON_String)
      strcpy(token->refresh, e->valuestring);
   else goto netatmo_parse_token_json_clean_exit;

   e=cJSON_GetObjectItem(response_cjson,"expire_in");
   if(e && e->type==cJSON_Number)
      token->expire_in=e->valueint;
   else goto netatmo_parse_token_json_clean_exit;

   ret_code=0;

netatmo_parse_token_json_clean_exit:
   if(ret_code==-1)
   {
      token->access[0]=0;
      token->refresh[0]=0;
      token->expire_in=0;
   }

   if(response_cjson)
   {
      cJSON_Delete(response_cjson);
      response_cjson=NULL;
   }

   return ret_code;
}


int netatmo_refresh_token(char *client_id, char *client_secret, struct netatmo_token_s *netatmo_token, char *err, int l_err)
{
   CURL *curl;
   char *url="https://api.netatmo.net/oauth2/token";
   char post_data[1024];

   if(netatmo_token->access[0]==0 || netatmo_token->refresh[0]==0)
   {
      DEBUG_SECTION mea_log_printf("%s (%s) : token not initialized (%d)\n", DEBUG_STR, __func__);
      return -1;
   }

   sprintf(post_data, "grant_type=refresh_token&client_id=%s&client_secret=%s&refresh_token=%s", client_id, client_secret, netatmo_token->refresh);

   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
         ret=_netatmo_parse_token_json(cr.p, netatmo_token, err, l_err);

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);

   return ret;
}


int netatmo_get_token(char *client_id, char *client_secret, char *username, char *password, char *scope, struct netatmo_token_s *netatmo_token, char *err, int l_err)
{
   CURL *curl;
   char *url="https://api.netatmo.net/oauth2/token";
   char post_data[1024];

   sprintf(post_data, "grant_type=password&client_id=%s&client_secret=%s&username=%s&password=%s&scope=%s", client_id,client_secret,username,password,scope);
     
   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
         ret=_netatmo_parse_token_json(cr.p, netatmo_token, err, l_err); 

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);

   return ret;
}


int netatmo_set_thermostat_setpoint(char *access_token, char *relay_id, char *thermostat_id, enum netatmo_setpoint_mode_e mode, uint32_t delay, double temp, char *err, int l_err)
{
   CURL *curl;
   char *api="https://api.netatmo.net/api/setthermpoint";
   char *_mode=NULL;
   char post_data[1024];
   time_t endtime=0;
   double _temp=0.0;

   if(delay!=0)
      endtime=time(NULL)+delay;

   switch(mode)
   {
      case PROGRAM:
         _mode="program";
         break;
      case AWAY:
         _mode="away";
         break;
      case HG:
         _mode="hg";
         break;
      case MANUAL:
         _mode="manual";
         if(temp<7.0)
         {
            if(err)
               snprintf(err,l_err,"temp (%3.0f °C) too low !\n", temp);
            return 1;
         }
         _temp=temp;
         break;
      case OFF:
         _mode="off";
         break;
      case MAX:
         _mode="max";
         if(delay==0)
            delay=3600;
         endtime=time(NULL)+delay;
         break;
      default:
         return -1;
   }

   sprintf(post_data, "access_token=%s&device_id=%s&module_id=%s&setpoint_mode=%s", access_token, relay_id, thermostat_id, _mode);
   if(_temp!=0.0)
   {
      char spt[5];
      sprintf(spt, "%04.1f", _temp);
      mea_snprintfcat(post_data, sizeof(post_data), "&setpoint_temp=%s", spt); 
   }
   if(endtime!=0)
      mea_snprintfcat(post_data, sizeof(post_data), "&setpoint_endtime=%d", endtime);

     struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, api);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         ret=-1;
      }
      else
      {
         ret=_netatmo_parse_return_json(cr.p, err, l_err);
      }

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);

   return ret;
}


int netatmo_get_thermostat_data(char *access_token, char *relay_id, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data, char *err, int l_err)
{
   CURL *curl;
   char *api="https://api.netatmo.net/api/getthermostatsdata";

   char url[1024];

   sprintf(url, "%s?access_token=%s&device_id=%s", api, access_token, relay_id);
     
   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
   {
      if(err)
         strncpy(err, "curl_result_init error", l_err);
      return -1;
   }

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

//      DEBUG_SECTION curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      CURLcode res = curl_easy_perform(curl);
      if(res != CURLE_OK)
      {
         DEBUG_SECTION mea_log_printf("%s (%s) : curl_easy_perform() failed (%d) - %s\n", DEBUG_STR, __func__, time(NULL), curl_easy_strerror(res));
         if(err)
            strncpy(err, curl_easy_strerror(res), l_err);
         ret=-1;
      }
      else
      {
         ret=_netatmo_parse_data_json(cr.p, thermostat_id, thermostat_data, err, l_err);
         if(ret<0)
         {
            DEBUG_SECTION mea_log_printf("%s\n", cr.p);
         }
      }

      curl_easy_cleanup(curl);
   }
   else
   {
      if(err)
         strncpy(err, "curl_easy_init error", l_err);
      ret=-1;
   }

   curl_result_release(&cr);
   return ret;
}


#ifdef NETATMO_MODULE_TEST
int main(int argc, char *argv[])
{
   struct netatmo_token_s token;
   struct netatmo_thermostat_data_s data;
   char err[80];
   int ret;

   ret=netatmo_get_token("563e5ce3cce37c07407522f2","lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4","patrice.dietsch@gmail.com","WEBcdpii10", "read_thermostat write_thermostat", &token, err, sizeof(err)-1);
   if(ret!=0)
   {
      if(ret<0)
         printf("Authentification error : %d\n", ret);
      else
         printf("Authentification error : %s (%d)\n", err, ret);
      exit(1);
   }


   ret=netatmo_refresh_token("563e5ce3cce37c07407522f2","lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4", &token, err, sizeof(err)-1);
   if(ret!=0)
   {
      if(ret<0)
         printf("Refresh error : %d\n", ret);
      else
         printf("Refresh error : %s (%d)\n", err, ret);
      exit(1);
   }


   ret=netatmo_get_thermostat_data(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", &data, err, sizeof(err)-1);
   if(ret==0)
      printf("temperature = %f %d\n", data.temperature,data.setpoint);
   else if(ret>0)
      printf("error : %s (%d)\n", err, ret); 

//   netatmo_set_thermostat_setpoint(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", MANUAL, 3600, 19.5, err, sizeof(err)-1);
//   netatmo_set_thermostat_setpoint(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", MAX, 3600, -1.0, err, sizeof(err)-1);
   netatmo_set_thermostat_setpoint(token.access, "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", AWAY, 3600, -1.0, err, sizeof(err)-1);
}
#endif

