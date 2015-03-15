/**********************************************************************
* Copyright (c) 2013-2014  Red Hat, Inc.
*
* Developed by Daynix Computing LTD.
*
* Authors:
*     Dmitry Fleytman <dmitry@daynix.com>
*     Pavel Gurvich <pavel@daynix.com>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
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

    class key_iterator
    {
    public:
        key_iterator(const UsbDkRegAccess &Root)
            : m_Root(&Root)
            , m_End(false)
        {
            advance();
        }

        key_iterator()
        {}

        const key_iterator& operator++()
        {
            advance();
            return *this;
        }

        bool operator !=(const key_iterator& other) const
        { return !m_End || !other.m_End; }

        LPCTSTR operator *()
        { return m_CurrentKeyName; }

       private:
        void advance();

        const            UsbDkRegAccess* m_Root = nullptr;
        TCHAR            m_CurrentKeyName[DEFAULT_REG_ENTRY_DATA_LEN] = {};
        int              m_NextIndex = 0;
        bool             m_End = true;
    };

    key_iterator begin() const
    { return  key_iterator(*this); }

    key_iterator end() const
    { return  key_iterator(); }

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
                      LPCTSTR lpzSubKey = nullptr) const;

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
                    LPCTSTR lpzSubKey = nullptr) const;
    BOOL  ReadDWord(LPCTSTR lpzValueName,
                    LPDWORD lpdwValue,
                    LPCTSTR lpzSubKey = nullptr) const;
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
                           LPCTSTR   lpzSubKey) const;

    LPTSTR m_lpsRegPath;
    HKEY   m_hkPrimaryHKey;
};
