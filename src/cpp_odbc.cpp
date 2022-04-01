///////////////////////////////////////////////////////////////////////////////
// Copyright 2022, Oushu Inc.
// All rights reserved.
//
// Author:
///////////////////////////////////////////////////////////////////////////////

#include "../include/cpp_odbc.h"
#include <string.h>

//init odbc handle variables
CppODBC::CppODBC() {
    V_OD_err_ = 0;
    bOpened_ = false;
    bConnected_ = false;
    nMaxFiledLen_ = 512;
    bEof_ = false;

    for (int i = 0; i < FIELD_NUM; i++)
        pszField_[i] = NULL;

    mutex_inited_ = false;

    if (!mutex_inited_) {
        pthread_mutex_init(&mutex_, NULL);
        mutex_inited_ = true;
    }
}

//clear handle
CppODBC::~CppODBC() {
    if (mutex_inited_)
        pthread_mutex_destroy(&mutex_);
    Clear();
}

bool CppODBC::Open() {
    if (bOpened_)
        return true;

    long V_OD_erg;
    // allocate Environment handle and register version
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &V_OD_Env_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error AllocHandle\n");
        return false;
    }

    V_OD_erg = SQLSetEnvAttr(V_OD_Env_, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error SetEnv\n");
        SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env_);
        return false;
    }

    bOpened_ = true;
    return true;
}

bool CppODBC::Close() {
    if (bConnected_)
        return false;

    SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env_);
    bOpened_ = false;
    return true;
}

bool CppODBC::Connect(const char *pszDSN, const char *pszUName, const char *pszUPasswd) {
    if (!bOpened_)
        return false;

    if (pszDSN == NULL)
        return false;

    long V_OD_erg = 0;

    SQLCHAR V_OD_stat[64] = {0}; // sql result status
    SQLSMALLINT V_OD_mlen = 0;  // error msg size
    SQLCHAR V_OD_msg[256] = {0};// error buffer

    // allocate connection handle, set timeout
    V_OD_erg = SQLAllocHandle(SQL_HANDLE_DBC, V_OD_Env_, &V_OD_hdbc_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error AllocHDB %ld\n", V_OD_erg);
        return false;
    }

    //(SQLPOINTER *)
    V_OD_erg = SQLSetConnectAttr(V_OD_hdbc_, SQL_LOGIN_TIMEOUT, (SQLPOINTER) 5, 0);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error SQLSetConnectAttr %ld\n", V_OD_erg);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc_);
        return false;
    }

    // Connect to the database
    V_OD_erg = SQLConnect(V_OD_hdbc_, (SQLCHAR *) pszDSN, SQL_NTS,
                          (SQLCHAR *) pszUName, SQL_NTS,
                          (SQLCHAR *) pszUPasswd, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error SQLConnect %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1,
                      V_OD_stat, &V_OD_err_, V_OD_msg, 256, &V_OD_mlen);
        printf("%s (%ld)\n", V_OD_msg, V_OD_err_);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc_);
        return false;
    }

    V_OD_erg = SQLAllocHandle(SQL_HANDLE_STMT, V_OD_hdbc_, &V_OD_hstmt_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Fehler im AllocStatement %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1, V_OD_stat, &V_OD_err_, V_OD_msg, 256, &V_OD_mlen);
        printf("%s (%ld)\n", V_OD_msg, V_OD_err_);
        SQLDisconnect(V_OD_hdbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc_);
        return false;
    }

    bConnected_ = true;
    return true;
}

bool CppODBC::DisConnect() {
    if (bConnected_) {
        SQLFreeHandle(SQL_HANDLE_STMT, V_OD_hstmt_);
        SQLDisconnect(V_OD_hdbc_);
        SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc_);
        bConnected_ = false;
    }

    return true;
}

//return result msg with sql commands
unsigned int CppODBC::SQLQuery(const char *pszSQL) {
    if (pszSQL == NULL)
        return 0;

    long V_OD_erg = 0;
    SQLCHAR V_OD_stat[64] = {0};
    SQLSMALLINT V_OD_mlen = 0;
    SQLCHAR V_OD_msg[256] = {0};
    char *pszBuf = NULL;

    Clear();

    //sql execute
    V_OD_erg = SQLExecDirect(V_OD_hstmt_, (SQLCHAR *) pszSQL, SQL_NTS);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error in Select %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1, V_OD_stat, &V_OD_err_, V_OD_msg, 256, &V_OD_mlen);
        printf("%s (%ld)\n", V_OD_msg, V_OD_err_);
        return 0;
    }

    //get query result
    V_OD_erg = SQLRowCount(V_OD_hstmt_, &V_OD_rowanz_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        return 0;
    }

    if (V_OD_rowanz_ == 0)
        return 0;

    //get result count
    V_OD_erg = SQLNumResultCols(V_OD_hstmt_, &V_OD_colanz_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        return 0;
    }

    printf("Number of Columns %d\n", V_OD_colanz_);

    char sz_buf[256] = {0};
    SQLSMALLINT buf_len = 0;
    SQLLEN colLen = 0;

    SQLLEN colInfo = 0;
    SQLLEN colType = 0;
    for (int i = 1; i <= V_OD_colanz_; i++)
    {
        SQLColAttribute(V_OD_hstmt_, i, SQL_DESC_NAME, sz_buf, 256, &buf_len, 0);
        SQLColAttribute(V_OD_hstmt_, i, SQL_DESC_TYPE, 0, 0, 0, &colType);

        SQLColAttribute(V_OD_hstmt_, i, SQL_DESC_LENGTH, NULL, 0, 0, &colLen);
        pszBuf = (char *) malloc(colLen + 1);

        memset(pszBuf, 0, colLen + 1);
        pszField_[i] = pszBuf;

        SQLLEN V_ERR;
        SQLBindCol(V_OD_hstmt_, i, SQL_C_CHAR, pszBuf, colLen, &V_ERR);

        printf("col %d szBuf %s, buflen is %d, len is %ld, type is %ld\n", i, sz_buf, buf_len, colLen, colType);
    }

    //get result and put into buffer
    V_OD_erg = SQLFetch(V_OD_hstmt_);

    if (V_OD_erg != SQL_NO_DATA)
        bEof_ = false;

    return V_OD_rowanz_;
}


bool CppODBC::Clear() {
    V_OD_rowanz_ = 0;
    V_OD_colanz_ = 0;
    bEof_ = true;

    for (int i = 0; i < FIELD_NUM; i++) {
        if (pszField_[i] != NULL)
            free(pszField_[i]);
        pszField_[i] = NULL;
    }
    return true;
}

//execute sql qurey and only get result count
unsigned int CppODBC::SQLExec(const char *pszSQL) {
    if (pszSQL == NULL)
        return 0;

    long V_OD_erg = 0;
    SQLCHAR V_OD_stat[64] = {0};
    SQLSMALLINT V_OD_mlen = 0;
    SQLCHAR V_OD_msg[256] = {0};

    Clear();

    V_OD_erg = SQLExecDirect(V_OD_hstmt_, (SQLCHAR *) pszSQL, SQL_NTS);

    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        printf("Error in Select %ld\n", V_OD_erg);
        SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc_, 1, V_OD_stat, &V_OD_err_, V_OD_msg, 256, &V_OD_mlen);
        printf("%s (%ld)\n", V_OD_msg, V_OD_err_);
        return 0;
    }

    V_OD_erg = SQLRowCount(V_OD_hstmt_, &V_OD_rowanz_);
    if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO)) {
        return 0;
    }

    return V_OD_rowanz_;
}

unsigned int CppODBC::SQLExecAutoID(char *pszSQL) {
    return 0;
}


bool CppODBC::IsOpen() {
    return bOpened_;
}

unsigned int CppODBC::GetCount() {
    return V_OD_rowanz_;
}

unsigned int CppODBC::GetColumns() {
    return V_OD_colanz_;
}

int CppODBC::GetIntValue(unsigned int uiIndex) {
    if (uiIndex < 0 || (short) uiIndex > V_OD_colanz_)
        return 0;

    int nField = 0;
    nField = atoi(pszField_[uiIndex]);

    return nField;
}

char *CppODBC::GetStrValue(unsigned int uiIndex) {
    if (uiIndex < 0 || (short) uiIndex > V_OD_colanz_)
        return NULL;

    return pszField_[uiIndex];
}

bool CppODBC::Cancel() {
    return true;
}

long CppODBC::GetError() {
    return V_OD_err_;
}

bool CppODBC::Next() {

    long V_OD_erg = 0;
    V_OD_erg = SQLFetch(V_OD_hstmt_);

    if (V_OD_erg != SQL_NO_DATA)
        bEof_ = false;
    else
        bEof_ = true;

    return !bEof_;
}

bool CppODBC::Eof() {
    return bEof_;
}

bool CppODBC::Lock() {
    if (!mutex_inited_)
        return false;

    pthread_mutex_lock(&mutex_);
    return true;
}

bool CppODBC::UnLock() {
    if (!mutex_inited_)
        return false;

    pthread_mutex_unlock(&mutex_);
    return true;
}
