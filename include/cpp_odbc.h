///////////////////////////////////////////////////////////////////////////////
// Copyright 2022, Oushu Inc.
// All rights reserved.
//
// Author:
///////////////////////////////////////////////////////////////////////////////

#ifndef _CPP_ODBC_H_

#define _CPP_ODBC_H_


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>

//max selected num
#define FIELD_NUM 1024

class CppODBC {
public:
    CppODBC();

    virtual ~CppODBC();

public:
    bool Open();

    bool Close();

    bool Connect(const char *pszDSN, const char *pszUName, const char *pszUPasswd);

    bool DisConnect();

    bool Clear();

    unsigned int SQLQuery(const char *pszSQL);

    unsigned int SQLExec(const char *pszSQL);

    unsigned int SQLExecAutoID(char *pszSQL);

    bool IsOpen();

    //get count of query result
    unsigned int GetCount();

    //get columns of query result
    unsigned int GetColumns();

    int GetIntValue(unsigned int uiIndex);

    char *GetStrValue(unsigned int uiIndex);

    //cancel qurey
    bool Cancel();

    //get error status
    long GetError();

    //next hadle
    bool Next();

    bool Eof();

    bool Lock();

    bool UnLock();


private:
    SQLHENV V_OD_Env_;    // Handle ODBC environment
    SQLHDBC V_OD_hdbc_;    // Handle connection
    SQLHSTMT V_OD_hstmt_;   // SQL handle
    SQLLEN V_OD_rowanz_;
    SQLSMALLINT V_OD_colanz_;   // sql colume
    SQLINTEGER V_OD_err_;  // sql error code
    char *pszField_[FIELD_NUM];  // qurey result buffer
    int nMaxFiledLen_;    // max size of query
    bool bOpened_;
    bool bConnected_;
    bool bEof_;

    pthread_mutex_t mutex_;
    bool mutex_inited_;
};

#endif
