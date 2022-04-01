#!/bin/bash/
# --- coding: utf-8 --- 
#Filename:   main.sh
#Date:       2022-03-22 11-41-56

if [[ -z $1 ]]; do
    echo "at least 1 parameter need"
done

case $1 in
run)
    sudo cp Dockerfile_run Dockerfile
    sudo docker build -t odbc:run .
    sudo docker run -v /home/gpadmin/docker_demo/CloudTest:/CloudTest odbc:run /bin/sh -c "cp /CloudTest/config/odbc.ini /etc/odbc.ini; /CloudTest/main"
    ;;
compile)
    sudo cp Dockerfile_compile Dockerfile
    sudo docker build -t odbc:compile .
    sudo docker run -v /home/gpadmin/docker_demo/CloudTest:/CloudTest odbc:compile -c "cp /CloudTest/config/odbc.ini /etc/odbc.ini; cd /CloudTest; make;"
    ;;
esac

