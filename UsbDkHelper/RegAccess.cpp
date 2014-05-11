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

// RegAccess.cpp: implementation of the UsbDkRegAccess class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RegAccess.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

UsbDkRegAccess::UsbDkRegAccess()
    : m_lpsRegPath(nullptr), m_hkPrimaryHKey(0)
{
}

UsbDkRegAccess::UsbDkRegAccess(HKEY hNewPrKey, LPCTSTR lpzNewRegPath)
    : m_lpsRegPath(nullptr), m_hkPrimaryHKey(0)
{
    SetPrimaryKey(hNewPrKey);
    if (SetRegPath(lpzNewRegPath) == FALSE)
    {
        throw UsbDkRegAccessConstructorFailedException();
    }
}

UsbDkRegAccess::~UsbDkRegAccess()
{
    delete m_lpsRegPath;
}

VOID UsbDkRegAccess::SetPrimaryKey(HKEY hNewPrKey)
{
    m_hkPrimaryHKey = hNewPrKey;
}

BOOL UsbDkRegAccess::SetRegPath(LPCTSTR lpzNewRegPath)
{
    delete m_lpsRegPath;

    if (!lpzNewRegPath)
    {
        m_lpsRegPath = nullptr;
        return TRUE;
    }

    m_lpsRegPath = _tcsdup(lpzNewRegPath);
    return (m_lpsRegPath != nullptr)?TRUE:FALSE;
}

HKEY UsbDkRegAccess::GetPrimaryKey(VOID)
{
    return m_hkPrimaryHKey;
}

BOOL UsbDkRegAccess::GetRegPath(LPTSTR lpsBuffer, DWORD dwNumberOfElements)
{
    if (!dwNumberOfElements)
    {
        return FALSE;
    }

    if (!m_lpsRegPath)
    {
        *lpsBuffer = 0;
        return TRUE;
    }

    return (_tcscpy_s(lpsBuffer, dwNumberOfElements, m_lpsRegPath) == 0)?TRUE:FALSE;
}

DWORD UsbDkRegAccess::ReadDWord(LPCTSTR lpzValueName,
                              DWORD   dwDefault,
                              LPCTSTR lpzSubKey)
{
    DWORD dwRes = 0;

    return (ReadDWord(lpzValueName, &dwRes, lpzSubKey) == TRUE)?dwRes:dwDefault;
}

BOOL UsbDkRegAccess::ReadDWord(LPCTSTR lpzValueName,
                             LPDWORD lpdwValue,
                             LPCTSTR lpzSubKey)
{
    BOOL  bRes = FALSE;
    DWORD dwValue = 0,
          dwSize = sizeof(dwValue),
          dwType = REG_DWORD;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_QUERY_VALUE,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkReadKeyHandle,
                            lpzValueName,
                            nullptr,
                            &dwType,
                            (LPBYTE)&dwValue,
                            &dwSize) == ERROR_SUCCESS)
        {
            bRes = TRUE;
            if (lpdwValue)
            {
                *lpdwValue = dwValue;
            }
        }

        RegCloseKey(hkReadKeyHandle);
    }

    return bRes;
}

DWORD UsbDkRegAccess::ReadString(LPCTSTR lpzValueName,
                               LPTSTR  lpzData,
                               DWORD   dwNumberOfElements,
                               LPCTSTR lpzSubKey)
{
    DWORD dwRes = 0;
    DWORD dwType = REG_SZ;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwBuffSize = dwNumberOfElements * sizeof(lpzData[0]);

    memset(lpzData, 0, dwBuffSize);

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_QUERY_VALUE,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkReadKeyHandle,
                            lpzValueName,
                            nullptr,
                            &dwType,
                            (LPBYTE)lpzData,
                            &dwBuffSize) == ERROR_SUCCESS)
            dwRes = dwBuffSize / sizeof(lpzData[0]);

        RegCloseKey(hkReadKeyHandle);
    }

    return dwRes;
}
//-----------------------------------------------------------------------------------------

LONG UsbDkRegAccess::ReadMultiString(LPCTSTR lpzValueName,
                                      LPTSTR  lpzData,
                                      DWORD   dwNumberOfElements,
                                      DWORD &dwRes,
                                      LPCTSTR lpzSubKey)
{
    dwRes = 0;
    DWORD dwType = REG_MULTI_SZ;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwBuffSize = dwNumberOfElements * sizeof(lpzData[0]);

    if (lpzData)
    {
        memset(lpzData, 0, dwBuffSize);
    }

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    LONG errorCode = RegOpenKeyEx(m_hkPrimaryHKey, tcaFullRegPath, 0, KEY_QUERY_VALUE, &hkReadKeyHandle);

    if (errorCode == ERROR_SUCCESS)
    {
        errorCode = RegQueryValueEx(hkReadKeyHandle, lpzValueName, nullptr, &dwType, (LPBYTE)lpzData, &dwBuffSize);

        if (errorCode == ERROR_SUCCESS)
        {
            dwRes = dwBuffSize / sizeof(lpzData[0]);
        }

        RegCloseKey(hkReadKeyHandle);
    }

    return errorCode;
}
//-----------------------------------------------------------------------------------------

DWORD UsbDkRegAccess::ReadBinary(LPCTSTR lpzValueName,
                               LPBYTE  lpzData,
                               DWORD   dwSize,
                               LPCTSTR lpzSubKey)
{
    DWORD dwRes = 0;
    DWORD dwType = REG_BINARY;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    memset(lpzData, 0, dwSize);

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_QUERY_VALUE,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkReadKeyHandle,
                            lpzValueName,
                            nullptr,
                            &dwType,
                            lpzData,
                            &dwSize) == ERROR_SUCCESS)
            dwRes = dwSize;

        RegCloseKey(hkReadKeyHandle);
    }

    return dwRes;
}

BOOL UsbDkRegAccess::ReadValueName(LPTSTR  lpsValueName,
                                 DWORD   dwNumberOfElements,
                                 DWORD   dwIndex,
                                 LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    BYTE baData[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwDataSize = DEFAULT_REG_ENTRY_DATA_LEN,
          dwType = REG_BINARY;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_QUERY_VALUE,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        DWORD dwBuffSize = dwNumberOfElements * sizeof(lpsValueName[0]);
        if (RegEnumValue(hkReadKeyHandle,
                         dwIndex,
                         lpsValueName,
                         &dwBuffSize,
                         nullptr,
                         &dwType,
                         baData,
                         &dwDataSize) == ERROR_SUCCESS)
            bResult = TRUE;
        RegCloseKey(hkReadKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::ReadKeyName(LPTSTR  lpsKeyName,
                               DWORD   dwNumberOfElements,
                               DWORD   dwIndex,
                               LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkReadKeyHandle = nullptr;
    FILETIME stTimeFile;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_ENUMERATE_SUB_KEYS,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        DWORD dwBuffSize = dwNumberOfElements * sizeof(lpsKeyName[0]);
        if (RegEnumKeyEx(hkReadKeyHandle,
                         dwIndex,
                         lpsKeyName,
                         &dwBuffSize,
                         nullptr,
                         nullptr,
                         nullptr,
                         &stTimeFile) == ERROR_SUCCESS)
            bResult = TRUE;
        RegCloseKey(hkReadKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::WriteValue(LPCTSTR lpzValueName,
                              DWORD   dwValue,
                              LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkWriteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwDisposition;

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegCreateKeyEx(m_hkPrimaryHKey,
                       tcaFullRegPath,
                       0,
                       TEXT(""),
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       nullptr,
                       &hkWriteKeyHandle,
                       &dwDisposition) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hkWriteKeyHandle,
                          lpzValueName,
                          0,
                          REG_DWORD,
                          (LPCBYTE)&dwValue,
                          sizeof(DWORD)) == ERROR_SUCCESS)
            bResult = TRUE;
        RegCloseKey(hkWriteKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::WriteString(LPCTSTR lpzValueName,
                               LPCTSTR lpzValue,
                               LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkWriteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwDisposition;

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegCreateKeyEx(m_hkPrimaryHKey,
                       tcaFullRegPath,
                       0,
                       TEXT(""),
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       nullptr,
                       &hkWriteKeyHandle,
                       &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD dwBuffSize = (DWORD)_tcslen(lpzValue) * sizeof(lpzValue[0]) + 1;
        if (RegSetValueEx(hkWriteKeyHandle,
                          lpzValueName,
                          0,
                          REG_SZ,
                          (LPCBYTE)lpzValue,
                          dwBuffSize) == ERROR_SUCCESS)
            bResult = TRUE;
        RegCloseKey(hkWriteKeyHandle);
    }

    return bResult;
}
//----------------------------------------------------------------------------------------------------

BOOL UsbDkRegAccess::WriteMultiString(LPCTSTR lpzValueName,
                                      LPCTSTR lpzValue,
                                      DWORD dwBuffSize,
                                      LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkWriteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwDisposition;

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegCreateKeyEx(m_hkPrimaryHKey,
        tcaFullRegPath,
        0,
        TEXT(""),
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        nullptr,
        &hkWriteKeyHandle,
        &dwDisposition) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hkWriteKeyHandle,
            lpzValueName,
            0,
            REG_MULTI_SZ,
            (LPCBYTE)lpzValue,
            dwBuffSize) == ERROR_SUCCESS)
        {
            bResult = TRUE;
        }
        RegCloseKey(hkWriteKeyHandle);
    }

    return bResult;
}
//----------------------------------------------------------------------------------------------------

BOOL UsbDkRegAccess::WriteBinary(LPCTSTR lpzValueName,
                               LPCBYTE lpData,
                               DWORD   dwDataSize,
                               LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkWriteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwDisposition;

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegCreateKeyEx(m_hkPrimaryHKey,
                       tcaFullRegPath,
                       0,
                       TEXT(""),
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       nullptr,
                       &hkWriteKeyHandle,
                       &dwDisposition) == ERROR_SUCCESS)
    {
        if (RegSetValueEx(hkWriteKeyHandle,
                          lpzValueName,
                          0,
                          REG_BINARY,
                          lpData,
                          dwDataSize) == ERROR_SUCCESS)
            bResult = TRUE;
        RegCloseKey(hkWriteKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::AddKey(LPCTSTR lpzKeyName)
{
    BOOL bResult = FALSE;
    HKEY hkWriteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];
    DWORD dwDisposition;

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzKeyName);

    if (RegCreateKeyEx(m_hkPrimaryHKey,
                       tcaFullRegPath,
                       0,
                       TEXT(""),
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       nullptr,
                       &hkWriteKeyHandle,
                       &dwDisposition) == ERROR_SUCCESS)
    {
        RegCloseKey(hkWriteKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::DeleteKey(LPCTSTR lpzKeyName, LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkDeleteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_WRITE,
                     &hkDeleteKeyHandle) == ERROR_SUCCESS)
    {
        if (RegDeleteKey(hkDeleteKeyHandle,
                         lpzKeyName) == ERROR_SUCCESS)
            bResult = TRUE;

        RegCloseKey(hkDeleteKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::DeleteValue(LPCTSTR lpzValueName, LPCTSTR lpzSubKey)
{
    BOOL bResult = FALSE;
    HKEY hkDeleteKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_WRITE,
                     &hkDeleteKeyHandle) == ERROR_SUCCESS)
    {
        if (RegDeleteValue(hkDeleteKeyHandle,
                           lpzValueName) == ERROR_SUCCESS)
            bResult = TRUE;
        RegCloseKey(hkDeleteKeyHandle);
    }

    return bResult;
}

BOOL UsbDkRegAccess::GetValueInfo(LPCTSTR lpzValueName,
                          DWORD* lpDataType,
                          DWORD* lpDataSize,
                          LPCTSTR lpzSubKey)
{
    BOOL bRet = FALSE;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_QUERY_VALUE,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkReadKeyHandle,
                            lpzValueName,
                            nullptr,
                            lpDataType,
                            nullptr,
                            lpDataSize) == ERROR_SUCCESS)
            bRet = TRUE;

        RegCloseKey(hkReadKeyHandle);
    }

    return bRet;
}

BOOL UsbDkRegAccess::GetKeyInfo(LPDWORD lpdwNofSubKeys,
                              LPDWORD lpdwMaxSubKeyLen,
                              LPDWORD lpdwNofValues,
                              LPDWORD lpdwMaxValueNameLen,
                              LPDWORD lpdwMaxValueLen,
                              LPCTSTR lpzSubKey)
{
    BOOL bRet = FALSE;
    HKEY hkReadKeyHandle = nullptr;
    TCHAR tcaFullRegPath[DEFAULT_REG_ENTRY_DATA_LEN];

    FormatFullRegPath(tcaFullRegPath, TBUF_SIZEOF(tcaFullRegPath), lpzSubKey);

    if (RegOpenKeyEx(m_hkPrimaryHKey,
                     tcaFullRegPath,
                     0,
                     KEY_QUERY_VALUE,
                     &hkReadKeyHandle) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hkReadKeyHandle,
                            nullptr,
                            nullptr,
                            nullptr,
                            lpdwNofSubKeys,
                            lpdwMaxSubKeyLen,
                            nullptr,
                            lpdwNofValues,
                            lpdwMaxValueNameLen,
                            lpdwMaxValueLen,
                            nullptr,
                            nullptr) == ERROR_SUCCESS)
            bRet = TRUE;

        RegCloseKey(hkReadKeyHandle);
    }

    return bRet;

}


VOID UsbDkRegAccess::FormatFullRegPath(LPTSTR lpzFullPathBuff, DWORD_PTR dwNumberOfElements, LPCTSTR lpzSubKey)
{
    DWORD_PTR dwReqNumberOfElements = (m_lpsRegPath?_tcslen(m_lpsRegPath):0) +
                                      (lpzSubKey?_tcslen(lpzSubKey):0) +
                                      ((m_lpsRegPath && lpzSubKey)?1:0) + 1;

    memset(lpzFullPathBuff, 0, dwNumberOfElements);
    if (dwNumberOfElements >= dwReqNumberOfElements)
    {
        if (m_lpsRegPath)
            _tcscpy_s(lpzFullPathBuff, dwNumberOfElements, m_lpsRegPath);

        if (lpzSubKey)
        {
            if (m_lpsRegPath)
                _tcscat_s(lpzFullPathBuff, dwNumberOfElements, TEXT("\\"));
            _tcscat_s(lpzFullPathBuff, dwNumberOfElements, lpzSubKey);
        }
    }
}
