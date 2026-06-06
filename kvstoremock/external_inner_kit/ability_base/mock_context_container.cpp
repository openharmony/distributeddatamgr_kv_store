/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "context_container.h"
namespace OHOS::AppExecFwk {

void ContextContainer::AttachBaseContext(const std::shared_ptr<ContextDeal> &base)
{

}

std::shared_ptr<ProcessInfo> ContextContainer::GetProcessInfo() const
{
    return std::shared_ptr<ProcessInfo>();
}

std::shared_ptr<ApplicationInfo> ContextContainer::GetApplicationInfo() const
{
    return std::shared_ptr<ApplicationInfo>();
}

std::shared_ptr<Context> ContextContainer::GetApplicationContext() const
{
    return std::shared_ptr<Context>();
}

std::string ContextContainer::GetBundleCodePath()
{
    return std::string();
}

const std::shared_ptr<AbilityInfo> ContextContainer::GetAbilityInfo()
{
    return std::shared_ptr<AbilityInfo>();
}

std::shared_ptr<Context> ContextContainer::GetContext()
{
    return std::shared_ptr<Context>();
}

std::shared_ptr<BundleMgrHelper> ContextContainer::GetBundleManager() const
{
    return std::shared_ptr<BundleMgrHelper>();
}

std::shared_ptr<Global::Resource::ResourceManager> ContextContainer::GetResourceManager() const
{
    return std::shared_ptr<Global::Resource::ResourceManager>();
}

std::string ContextContainer::GetDatabaseDir()
{
    return std::string();
}

std::string ContextContainer::GetDataDir()
{
    return std::string();
}

std::string ContextContainer::GetDir(const std::string &name, int mode)
{
    return std::string();
}

std::string ContextContainer::GetFilesDir()
{
    return std::string();
}

std::string ContextContainer::GetBundleName() const
{
    return std::string();
}

std::string ContextContainer::GetBundleResourcePath()
{
    return std::string();
}

sptr<AAFwk::IAbilityManager> ContextContainer::GetAbilityManager()
{
    return sptr<AAFwk::IAbilityManager>();
}

std::string ContextContainer::GetAppType()
{
    return std::string();
}

void ContextContainer::SetPattern(int patternId)
{

}

std::shared_ptr<HapModuleInfo> ContextContainer::GetHapModuleInfo()
{
    return std::shared_ptr<HapModuleInfo>();
}

std::string ContextContainer::GetProcessName()
{
    return std::string();
}

std::shared_ptr<Context> ContextContainer::CreateBundleContext(const std::string &bundleName, int flag, int accountId)
{
    return std::shared_ptr<Context>();
}

Uri ContextContainer::GetCaller()
{
    return Uri("");
}

void ContextContainer::InitResourceManager(BundleInfo &bundleInfo, std::shared_ptr<ContextDeal> &deal)
{

}

std::string ContextContainer::GetString(int resId)
{
    return std::string();
}

std::vector<std::string> ContextContainer::GetStringArray(int resId)
{
    return std::vector<std::string>();
}

std::vector<int> ContextContainer::GetIntArray(int resId)
{
    return std::vector<int>();
}

std::map<std::string, std::string> ContextContainer::GetTheme()
{
    return std::map<std::string, std::string>();
}

void ContextContainer::SetTheme(int themeId)
{

}

std::map<std::string, std::string> ContextContainer::GetPattern()
{
    return std::map<std::string, std::string>();
}

int ContextContainer::GetColor(int resId)
{
    return 0;
}

int ContextContainer::GetThemeId()
{
    return 0;
}

int ContextContainer::GetDisplayOrientation()
{
    return 0;
}

std::string ContextContainer::GetPreferencesDir()
{
    return std::string();
}

void ContextContainer::SetColorMode(int mode)
{

}

int ContextContainer::GetColorMode()
{
    return 0;
}

int ContextContainer::GetMissionId()
{
    return 0;
}
}
