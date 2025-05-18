#!/bin/bash
echo "Старт сервера: $(date)" >> server.log 2>&1
nohup ./server >> server.log 2>&1 &
echo $! > server.pid
echo "Сервер запущен на http://localhost:8081"
