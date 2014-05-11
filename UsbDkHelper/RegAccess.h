/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* This work is licensed under the terms of the BSD license. See
* the LICENSE file in the top-level directory.
*
**********************************************************************/

// RegAccess.h: interface for the UsbDkRegAccess class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

const DWORD DEFAULT_REG_ENTRY_DATA_LEN  = 0x00000100;

class UsbDkRegAccessConstructorFailedException : public UsbDkException
{
public:
    UsbDkRegAccessConstructorFailedException() :
        UsbDkException(TEXT("Can't construct UsbDkRegAccess object"))
    {}
};

class UsbDkRegAccess
{
public:
    UsbDkRegAccess();
    UsbDkRegAccess(HKEY hNewPrKey, LPCTSTR lpzNewRegPath);
    virtual ~UsbDkRegAccess();

    VOID  SetPrimaryKey(HKEY hNewPrKey);
    BOOL  SetRegPath(LPCTSTR lpzNewRegPath);
    HKEY  GetPrimaryKey(VOID);
    BOOL  GetRegPath(LPTSTR lpsBuffer,
                     DWORD  dwNumberOfElements);

    BOOL  ReadValueName(LPTSTR lpsValueName,
                        DWORD  dwNumberOfElements,
                        DWORD  dwIndex=0,
                        LPCTSTR lpzSubKey=nullptr);
    BOOL  ReadKeyName(LPTSTR  lpsKeyName,
                      DWORD   dwNumberOfElements,
                      DWORD   dwIndex,
                      LPCTSTR lpzSubKey = nullptr);

    BOOL GetValueInfo(LPCTSTR lpzValueName,
                      DWORD*  lpDataType,
                      DWORD*  lpDataSize,
                      LPCTSTR lpzSubKey = nullptr);
    BOOL GetKeyInfo(LPDWORD lpdwNofSubKeys,
                    LPDWORD lpdwMaxSubKeyLen,
                    LPDWORD lpdwNofValues,
                    LPDWORD lpdwMaxValueNameLen,
                    LPDWORD lpdwMaxValueLen,
                    LPCTSTR lpzSubKey = nullptr);

    DWORD ReadDWord(LPCTSTR lpzValueName,
                    DWORD   dwDefault = 0,
                    LPCTSTR lpzSubKey = nullptr);
    BOOL  ReadDWord(LPCTSTR lpzValueName,
                    LPDWORD lpdwValue,
                    LPCTSTR lpzSubKey = nullptr);
    DWORD ReadString(LPCTSTR lpzValueName,
                     LPTSTR  lpzData,
                     DWORD   dwNumberOfElements,
                     LPCTSTR lpzSubKey=nullptr);
    LONG ReadMultiString(LPCTSTR lpzValueName,
                          LPTSTR  lpzData,
                          DWORD   dwNumberOfElements,
                          DWORD   &dwRes,
                          LPCTSTR lpzSubKey = nullptr);
    DWORD ReadBinary(LPCTSTR lpzValueName,
                     LPBYTE  lpzData,
                     DWORD   dwSize,
                     LPCTSTR lpzSubKey=nullptr);
    BOOL  WriteValue(LPCTSTR lpzValueName,
                     DWORD  dwValue,
                     LPCTSTR lpzSubKey = nullptr);
    BOOL  WriteString(LPCTSTR lpzValueName,
                      LPCTSTR lpzValue,
                      LPCTSTR lpzSubKey=nullptr);
    BOOL  WriteMultiString(LPCTSTR lpzValueName,
                           LPCTSTR lpzValue,
                           DWORD dwBuffSize,
                           LPCTSTR lpzSubKey = nullptr);
    BOOL  WriteBinary(LPCTSTR lpzValueName,
                      LPCBYTE lpData,
                      DWORD   dwDataSize,
                      LPCTSTR lpzSubKey = nullptr);
    BOOL  DeleteKey(LPCTSTR lpzKeyName,
                    LPCTSTR lpzSubKey = nullptr);
    BOOL  DeleteValue(LPCTSTR lpzValueName,
                      LPCTSTR lpzSubKey = nullptr);

    BOOL  AddKey(LPCTSTR lpzKeyName);

protected:
    VOID FormatFullRegPath(LPTSTR    lpzFullPathBuff,
                           DWORD_PTR dwNumberOfElements,
                           LPCTSTR   lpzSubKey);

    LPTSTR m_lpsRegPath;
    HKEY   m_hkPrimaryHKey;
};
