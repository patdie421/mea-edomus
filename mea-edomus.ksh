#!/bin/bash

while true
do
  echo `date` : start >> /data/mea-edomus/mea-edomus.log
  /data/mea-edomus/mea-edomus -a /data/mea-edomus/params.db
  echo `date` : exit mea-edomus >> /data/mea-edomus/mea-edomus.log
  sleep 1
done
