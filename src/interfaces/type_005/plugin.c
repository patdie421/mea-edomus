#include <stdio.h>
#include <dlfcn.h>

#include "interfacesServer.h"
#include "interface_type_005.h"

// dlsym(interface_type_005_lib, "get_type_interface_type_005_PLGN");

int get_fns_interface(void *lib, struct interfacesServer_interfaceFns_s *interfacesFns)
{
   if(lib)
   {
      interfacesFns->malloc_and_init_interface = (malloc_and_init_interface_f)dlsym(lib, "malloc_and_init_interface_type_005_PLGN");
      if(!interfacesFns->malloc_and_init_interface)
         fprintf(stderr,"malloc_and_init_interface: ", dlerror());

      interfacesFns->get_monitoring_id = (get_monitoring_id_f)dlsym(lib, "get_monitoring_id_interface_type_005_PLGN");
      if(!interfacesFns->get_monitoring_id)
         fprintf(stderr,"get_monitoring_id: ", dlerror());

      interfacesFns->get_xPLCallback = (get_xPLCallback_f)dlsym(lib, "get_xPLCallback_interface_type_005_PLGN");
      if(!interfacesFns->get_xPLCallback)
         fprintf(stderr,"get_xPLCallback: ", dlerror());

      interfacesFns->clean = (clean_f)dlsym(lib, "clean_interface_type_005_PLGN");
      if(!interfacesFns->get_xPLCallback)
         fprintf(stderr,"get_xPLCallback: ", dlerror());

      interfacesFns->set_monitoring_id = (set_monitoring_id_f)dlsym(lib, "set_monitoring_id_interface_type_005_PLGN");
      if(!interfacesFns->set_monitoring_id)
         fprintf(stderr,"set_monitoring_id: ", dlerror());

      interfacesFns->set_xPLCallback = (set_xPLCallback_f)dlsym(lib, "set_xPLCallback_interface_type_005_PLGN");
      if(!interfacesFns->set_xPLCallback)
         fprintf(stderr,"set_xPLCallback: ", dlerror());

      interfacesFns->get_type = (get_type_f)dlsym(lib, "get_type_interface_type_005_PLGN");
      if(!interfacesFns->get_type)
         fprintf(stderr,"get_type: ", dlerror());
         
      return 0;
   }
   else
   {
      return -1;
   }
}
