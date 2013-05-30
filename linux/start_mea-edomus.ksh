#!/bin/bash

PARAM="-s 192.168.0.22 \
-b domotique \
-u domotique \
-p passwd \
-a /data/mea-edomus/params.db \
-l /data/mea-edomus/queries.db"

# nohup ./mea-edomus $PARAM &

./mea-edomus $PARAM
