#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#include "cJSON.h"

#include "curl_adds.h"

#include "mea_verbose.h"


char *token_test="{\"access_token\":\"563de4bb2baa3cbb89752192|1eaa34f6cb5e7b56db2119d457368a19\",\"refresh_token\":\"563de4bb2baa3cbb89752192|4af35da41620a0f30af806c1d83872fe\",\"scope\":[\"read_thermostat\",\"write_thermostat\"],\"expires_in\":10800,\"expire_in\":10800}";


struct netatmo_token_s
{
   char access[81];
   char refresh[81]; 
   time_t expire_in;
};


struct netatmo_thermostat_data_s
{
   int battery_vp;
   int therm_relay_cmd;
   double temperature;
   double setpoint_temp;
};


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
          DEBUG_SECTION mea_log_printf("%s (%s) : NETATMO error : %s\n", DEBUG_STR, __func__, a->valuestring);
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

   e=cJSON_GetObjectItem(response_cjson, "body"); 
   if(e==NULL)
      goto _netatmo_parse_data_json_clean_exit;

   e=cJSON_GetObjectItem(e, "devices");
   if(e==NULL || e->type!=cJSON_Array)
       goto _netatmo_parse_data_json_clean_exit;

   e=cJSON_GetArrayItem(e, 0);
   if(e==NULL)
       goto _netatmo_parse_data_json_clean_exit;

   e=cJSON_GetObjectItem(e, "modules");
   if(e==NULL || e->type!=cJSON_Array)
       goto _netatmo_parse_data_json_clean_exit;
 
   int i=0;
   cJSON *a;
   do
   {
      a=cJSON_GetArrayItem(e, i); 
      if(a)
      {
         cJSON *e;
         e=cJSON_GetObjectItem(a, "_id");
         if(e==NULL)
            goto _netatmo_parse_data_json_clean_exit;
         if(strcmp(e->valuestring, thermostat_id)==0)
         {
            e=cJSON_GetObjectItem(a, "battery_vp");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->battery_vp=e->valueint;

            e=cJSON_GetObjectItem(a, "therm_relay_cmd");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->therm_relay_cmd=e->valueint;


            cJSON *mesured=cJSON_GetObjectItem(a, "measured");
            if(mesured==NULL)
               goto _netatmo_parse_data_json_clean_exit;

            e=cJSON_GetObjectItem(mesured, "temperature");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
            thermostat_data->temperature=e->valuedouble;

            e=cJSON_GetObjectItem(mesured, "setpoint_temp");
            if(e==NULL)
               goto _netatmo_parse_data_json_clean_exit;
             thermostat_data->setpoint_temp=e->valuedouble;
            ret_code=0;
            break;
         }
         i++;
      }
   }
   while(a);
   if(ret_code<0)
   {
      ret_code=9998;
      if(err)
         strcpy(err,"thermostat id not found");
   }

_netatmo_parse_data_json_clean_exit:
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


int netatmo_get_token(char *client_id, char *client_secret, char *username, char *password, char *scope, struct netatmo_token_s *netatmo_token, char *err, int l_err)
{
   CURL *curl;
   char *url="https://api.netatmo.net/oauth2/token";
   int ret_code=-1;
   char post_data[255];

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

      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, (curl_write_callback)curl_result_get);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cr);

      DEBUG_SECTION mea_log_printf("%s (%s) : try authentification ... (%d)\n", DEBUG_STR, __func__,time(NULL));

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


int netatmo_get_data(char *access_token, char *relay_id, char *thermostat_id, struct netatmo_thermostat_data_s *thermostat_data, char *err, int l_err)
{
   CURL *curl;
   char *_url="https://api.netatmo.net/api/getthermostatsdata";

   char url[255];

   sprintf(url, "%s?access_token=%s&device_id=%s", _url, access_token, relay_id);
     
   struct curl_result_s cr;
   int ret=-1;

   ret=curl_result_init(&cr);
   if(ret<0)
      return -1;

   curl = curl_easy_init();
   if(curl)
   {
      curl_easy_setopt(curl, CURLOPT_URL, url);

//      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
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
         ret=-1;
      }
      else
         ret=_netatmo_parse_data_json(cr.p, thermostat_id, thermostat_data, err, l_err);

      curl_easy_cleanup(curl);
   }
   else
      ret=-1;

   curl_result_release(&cr);
   return ret;
}


int main(int argc, char *argv[])
{
   struct netatmo_token_s token;
   struct netatmo_thermostat_data_s data;
   char err[80];

//   netatmo_get_token("563e5ce3cce37c07407522f2","lE1CUF1k3TxxSceiPpmIGY8QXJWIeXJv0tjbTRproMy4","patrice.dietsch@gmail.com","WEBcdpii10","read_thermostat write_thermostat", &token, err, sizeof(err)-1);

   int ret=netatmo_get_data("563de4bb2baa3cbb89752192|218d344476bf32f17ebcb08f685c31d3", "70:ee:50:0a:34:e0", "04:00:00:0a:37:8c", &data, err, sizeof(err)-1);

   if(ret==0)
      printf("temperature = %f\n", data.temperature);
   else if(ret>0)
      printf("error : %s (%d)\n", err, ret); 
}

