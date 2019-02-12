#include "stdafx.h"
#include "UsbDkDataHider.h"
#include "UsbDkNames.h"
#include "HideRulesRegPublic.h"
#include "RegAccess.h"
#include "RuleManager.h"
#include "GuidGen.h"

CRulesManager::CRulesManager()
    : m_RegAccess(HKEY_LOCAL_MACHINE, USBDK_HIDE_RULES_PATH)
{}

static bool operator == (const USB_DK_HIDE_RULE& r1, const USB_DK_HIDE_RULE& r2)
{
    return (r1.VID == r2.VID)     &&
           (r1.PID == r2.PID)     &&
           (r1.BCD == r2.BCD)     &&
           (r1.Class == r2.Class) &&
           (r1.Hide == r2.Hide)   &&
           (r1.Type == r2.Type);
}

DWORD CRulesManager::ReadDword(LPCTSTR RuleName, LPCTSTR ValueName) const
{
    DWORD RawValue;

    if (!m_RegAccess.ReadDWord(ValueName, &RawValue, RuleName))
    {
        tstring ErrorText = tstring(TEXT("Failed to read rule ")) + ValueName;
        throw UsbDkRuleManagerException(ErrorText, ERROR_FUNCTION_FAILED);
    }

    return RawValue;
}

void CRulesManager::WriteDword(const tstring &RuleName, LPCTSTR ValueName, ULONG Value)
{
    if (!m_RegAccess.WriteValue(ValueName, Value, RuleName.c_str()))
    {
        tstring ErrorText = tstring(TEXT("Failed to write rule ")) + ValueName;
        throw UsbDkRuleManagerException(ErrorText, ERROR_FUNCTION_FAILED);
    }
}

ULONG64 CRulesManager::ReadDwordMask(LPCTSTR RuleName, LPCTSTR ValueName) const
{
    return HideRuleUlongMaskFromRegistry(ReadDword(RuleName, ValueName));
}

ULONG64 CRulesManager::ReadBool(LPCTSTR RuleName, LPCTSTR ValueName) const
{
    return HideRuleBoolFromRegistry(ReadDword(RuleName, ValueName));
}

void CRulesManager::ReadRule(LPCTSTR RuleName, USB_DK_HIDE_RULE &Rule) const
{
    Rule.Type  = ReadDword(RuleName, USBDK_HIDE_RULE_TYPE);
    Rule.Hide  = ReadBool(RuleName, USBDK_HIDE_RULE_SHOULD_HIDE);
    Rule.VID   = ReadDwordMask(RuleName, USBDK_HIDE_RULE_VID);
    Rule.PID   = ReadDwordMask(RuleName, USBDK_HIDE_RULE_PID);
    Rule.BCD   = ReadDwordMask(RuleName, USBDK_HIDE_RULE_BCD);
    Rule.Class = ReadDwordMask(RuleName, USBDK_HIDE_RULE_CLASS);
}

template <typename TFunctor>
bool CRulesManager::FindRule(const USB_DK_HIDE_RULE &Rule, TFunctor Functor)
{
    for (const auto &SubKey : m_RegAccess)
    {
        try
        {
            USB_DK_HIDE_RULE ExistingRule;
            ReadRule(SubKey, ExistingRule);

            if (Rule == ExistingRule)
            {
                Functor(SubKey);
                return true;
            }
        }
        catch (const UsbDkRuleManagerException &e)
        {
            auto ErrorText = tstring(TEXT("Error while processing rule ")) +
                             SubKey + TEXT(": ") + string2tstring(e.what());
            OutputDebugString(ErrorText.c_str());
        }
    }

    return false;
}

bool CRulesManager::RuleExists(const USB_DK_HIDE_RULE &Rule)
{
    return FindRule(Rule, [](LPCTSTR){});
}

void CRulesManager::AddRule(const USB_DK_HIDE_RULE &Rule)
{
    if (RuleExists(Rule))
    {
        throw UsbDkRuleManagerException(TEXT("Rule already exists"), ERROR_FILE_EXISTS);
    }

    CGuid RuleName;

    if (!m_RegAccess.AddKey(RuleName))
    {
        throw UsbDkRuleManagerException(TEXT("Failed to create rule key"), ERROR_FUNCTION_FAILED);
    }

    WriteDword(RuleName, USBDK_HIDE_RULE_TYPE, static_cast<ULONG>(Rule.Type));
    WriteDword(RuleName, USBDK_HIDE_RULE_SHOULD_HIDE, static_cast<ULONG>(Rule.Hide));
    WriteDword(RuleName, USBDK_HIDE_RULE_VID, static_cast<ULONG>(Rule.VID));
    WriteDword(RuleName, USBDK_HIDE_RULE_PID, static_cast<ULONG>(Rule.PID));
    WriteDword(RuleName, USBDK_HIDE_RULE_BCD, static_cast<ULONG>(Rule.BCD));
    WriteDword(RuleName, USBDK_HIDE_RULE_CLASS, static_cast<ULONG>(Rule.Class));
}

void CRulesManager::DeleteRule(const USB_DK_HIDE_RULE &Rule)
{
    tstring RuleName;

    if (FindRule(Rule, [&RuleName](LPCTSTR Name){ RuleName = Name; }))
    {
        if (!m_RegAccess.DeleteKey(RuleName.c_str()))
        {
            throw UsbDkRuleManagerException(TEXT("Failed to delete rule key"), ERROR_FUNCTION_FAILED);
        }
    }
}

ULONG CRulesManager::DeleteAllRules(ULONG& notDeleted)
{
    ULONG deleted = 0;
    notDeleted = 0;
    vector<wstring> subkeys;

    for (const auto &SubKey : m_RegAccess)
        subkeys.push_back(SubKey);

    while (subkeys.size())
    {
        m_RegAccess.DeleteKey(subkeys.front().c_str()) ? deleted++ : notDeleted++;
        subkeys.erase(subkeys.begin());
    }
    return deleted;
}
