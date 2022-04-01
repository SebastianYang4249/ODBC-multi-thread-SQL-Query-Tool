///////////////////////////////////////////////////////////////////////////////
// Copyright 2022, Oushu Inc.
// All rights reserved.
//
// Author:
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#include "thread_pool.cpp"
#include "../include/yaml.h"
#include "../include/data.h"
#include "../include/cpp_odbc.h"

FILE *resLog;
int cmdIndex = 0;
int numOfParallel = 0;
std::string _user;
std::string _pass;

//spilit string to one sql and one result
void split(std::string &t, std::string &s1, std::string &s2) {
    int l = t.length();
    int i = t.rfind(' ');
    s1 = t.substr(0, i);
    s2 = t.substr(i + 1);
}

//read config from config/config.yaml(thread_pool and odbc config) and config/cmd.txt(sql command)
void readConfig(Data &data, std::vector <std::string> &cmds, std::vector<int> &sqlres) {
    FILE *fp;
    fp = fopen("config/config.yaml", "r");
    if (!fp) {
        std::fputs("cannot open config, using default configures", resLog);
        data.hostIp = "all";
        data.port = "7432";
        data.database = "postgres";
        data.userName = "cloud_test@oushutest";
        data.password = "ld4387vlobg";
        data.configOption = 1;
        data.parallelOption = 1;
        data.numberOfParallelUsers = 10000;
        data.numOfUser = 1;

        data.bufferSize = 100;
        data.threadNum = 8;
    } else {
        std::fputs("using customization settings\n", resLog);
        YAML::Node config = YAML::LoadFile("config/config.yaml");

        data.hostIp = config["hostIp"].as<std::string>();
        data.port = config["port"].as<std::string>();
        data.database = config["database"].as<std::string>();
        data.userName = config["userName"].as<std::string>();
        data.password = config["password"].as<std::string>();
        _user = data.userName;
        _pass = data.password;

        data.tolerant = config["tolerant"].as<int>();
        data.bufferSize = config["bufferSize"].as<int>();
        data.threadNum = config["threadNum"].as<int>();
        data.numberOfUsers = config["numberOfUsers"].as<int>();

        char tmp[128];
        std::string command, res;
        fp = fopen("config/cmd.txt", "r");
        while (fgets(tmp, sizeof(tmp), fp) != NULL) {
            if (tmp[strlen(tmp) - 1] == '\n') {
                tmp[strlen(tmp) - 1] = '\0';
            }
            std::string _tmp = tmp;
            split(_tmp, command, res);
            cmds.push_back(command);
            sqlres.push_back(std::stoi(res));
            data.numOfCmds++;
        }

        FILE *odbcConfig = fopen("config/odbc.ini", "w");
        if (!odbcConfig) {
            printf("No odbc.ini found\n");
            exit(0);
        }
        fprintf(odbcConfig,
                "[oushu]\nDescription = Connect to oushudb\nDriver = PostgreSQL\nServername = %s\nPort = %s\nDatabase = %s\nUserName = %s\nPassword = %s\nReadOnly = 0\nConnSettings = set client_encoding to UTF8",
                data.hostIp.c_str(), data.port.c_str(), data.database.c_str(), data.userName.c_str(),
                data.password.c_str());
        fclose(odbcConfig);
        system("sudo cp config/odbc.ini /etc/odbc.ini");
    }
}

//execute sql command in odbc
void sqlexec(std::vector <std::string> cmd, int cmdCount, std::vector<int> res) {
    CppODBC cppOdbc;
    bool bRes = cppOdbc.Open();

    if (!bRes) {
        fprintf(resLog, "thread : %lu, test: %d, fail, ODBC open error!\n", std::this_thread::get_id(), cmdIndex);
        cmdIndex++;
        return;
    }

    int flag = 1;

    for (int k = 0; k < cmdCount; ++k) {

        bRes = cppOdbc.Connect("oushu", _user.c_str(), _pass.c_str());
        if (!bRes) {
            fprintf(resLog, "thread : %lu, test: %d, fail, Connect error!\n", std::this_thread::get_id(), cmdIndex);
            cmdIndex++;
            return;
        }

        if (res[k] != cppOdbc.SQLExec(cmd[k].c_str())) {
            flag = 0;
        }

        cppOdbc.DisConnect();
    }

    if (flag) {
        fprintf(resLog, "thread : %lu, test: %d, pass\n", std::this_thread::get_id(), cmdIndex);
    } else {
        fprintf(resLog, "thread : %lu, test: %d, fail, Result wrong!\n", std::this_thread::get_id(), cmdIndex);
    }
    cmdIndex++;

    cppOdbc.Close();
    if (cmdIndex == numOfParallel) {
        printf("done\n");
        fclose(resLog);
        exit(0);
    }
}

int main() {
    std::vector <std::string> cmds;
    std::vector<int> res;

    Data data;

    CppODBC testConnect;
    testConnect.Open();
    testConnect.Connect("oushu", _user.c_str(), _pass.c_str());
    testConnect.Close();

    resLog = fopen("resLog", "w");

    readConfig(data, cmds, res);

    thread_pool pool{data.bufferSize};

    pool.start(data.threadNum);

    pool.setCmd(cmds, res, 10, data.numOfCmds);
    for (int i = 0; i < data.numberOfParallelUsers; ++i) {
        pool.submit(sqlexec);
    }

    sleep(data.tolerant);
    fclose(resLog);
    return 0;
}
