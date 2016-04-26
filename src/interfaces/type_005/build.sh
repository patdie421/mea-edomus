gcc -shared -Wl,--export-dynamic -g -O2 -std=gnu99 -I../..  -I/usr/include/mysql plugin.c interface_type_005.c -o interface_type_005.so -DASPLUGIN ; cp interface_type_005.so /tmp
