///////////////////////////////////////////////////////////////////////////////
// Copyright 2022, Oushu Inc.
// All rights reserved.
//
// Author:
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>

#ifndef CLOUDTEST_DATA_H
#define CLOUDTEST_DATA_H

struct Data{
    std::string hostIp;
    std::string port;
    std::string database;
    std::string userName;
    std::string password;
    // connection

    int tolerant;

    int bufferSize;
    int threadNum;
    int numberOfUsers;

    int numOfUser = 0;
    int numOfCmds = 0;
};

#endif //CLOUDTEST_DATA_H
