#pragma once
#define RULE_MANAGER_EXCEPTION_STRING TEXT("RuleManager exception: ")

class UsbDkRuleManagerException : public UsbDkW32ErrorException
{
public:
    UsbDkRuleManagerException() : UsbDkW32ErrorException(RULE_MANAGER_EXCEPTION_STRING){}
    UsbDkRuleManagerException(LPCTSTR lpzMessage) : UsbDkW32ErrorException(tstring(RULE_MANAGER_EXCEPTION_STRING) + lpzMessage){}
    UsbDkRuleManagerException(LPCTSTR lpzMessage, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(RULE_MANAGER_EXCEPTION_STRING) + lpzMessage, dwErrorCode){}
    UsbDkRuleManagerException(tstring errMsg) : UsbDkW32ErrorException(tstring(RULE_MANAGER_EXCEPTION_STRING) + errMsg){}
    UsbDkRuleManagerException(tstring errMsg, DWORD dwErrorCode) : UsbDkW32ErrorException(tstring(RULE_MANAGER_EXCEPTION_STRING) + errMsg, dwErrorCode){}
};

class CRulesManager
{
public:
    CRulesManager();

    void AddRule(const USB_DK_HIDE_RULE &Rule);
    void DeleteRule(const USB_DK_HIDE_RULE &Rule);
    ULONG DeleteAllRules(ULONG& notDeleted);
private:
    template <typename TFunctor>
    bool FindRule(const USB_DK_HIDE_RULE &Rule, TFunctor Functor);
    bool RuleExists(const USB_DK_HIDE_RULE &Rule);

    DWORD ReadDword(LPCTSTR RuleName, LPCTSTR ValueName) const;
    ULONG64 ReadDwordMask(LPCTSTR RuleName, LPCTSTR ValueName) const;
    ULONG64 ReadBool(LPCTSTR RuleName, LPCTSTR ValueName) const;
    void WriteDword(const tstring &RuleName, LPCTSTR ValueName, ULONG Value);

    void ReadRule(LPCTSTR RuleName, USB_DK_HIDE_RULE &Rule) const;

    UsbDkRegAccess m_RegAccess;
};
