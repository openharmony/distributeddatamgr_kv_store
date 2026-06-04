/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_ABILITY_BASE_USER_OBJECT_BASE_H
#define OHOS_ABILITY_BASE_USER_OBJECT_BASE_H
#include <unordered_map>
#include "parcel.h"
#include "string_ex.h"
namespace OHOS {
namespace AAFwk {
class UserObjectBase : public Parcelable {
public:
    UserObjectBase(const std::string &className) : className_(className)
    {}
    virtual ~UserObjectBase() = default;

    void SetClassName(const std::string &className)
    {
        className_ = className;
    }
    std::string GetClassName(void)
    {
        return className_;
    }
    /**
     * @brief The current object parameter is converted to a string
     * @param none
     * @return returns string of current object parameter
     */
    virtual std::string ToString() const = 0;

    /**
     * @brief The current object parameter is converted to a string
     * @param none
     * @return returns string of current object parameter
     */
    virtual void Parse(const std::string &str) = 0;

    /**
     * @brief Copy the data of the specified object to the current object with deepcopy
     */
    virtual void DeepCopy(std::shared_ptr<UserObjectBase> &other) = 0;

    /**
     * @brief Indicates whether some other object is "equal to" this one.
     * @param other The object with which to compare.
     * @return true if this object is the same as the obj argument; false otherwise.
     */
    virtual bool Equals(std::shared_ptr<UserObjectBase> &other) = 0;

    /**
     * @brief Writes a parcelable object to the given parcel.
     * @param parcel indicates Parcel object
     * @return Returns true being written on success or false if any error occur.
     */
    virtual bool Marshalling(Parcel &parcel) const
    {
        std::string tostring = ToString();
        return parcel.WriteString16(Str8ToStr16(tostring));
    }
    /**
     * @brief Read a parcelable object to the given parcel.
     * @param parcel indicates Parcel object
     * @return Returns true being read on success or false if any error occur.
     */
    virtual bool Unmarshalling(Parcel &parcel)
    {
        std::string tostring = Str16ToStr8(parcel.ReadString16());
        Parse(tostring);
        return true;
    }

protected:
    std::string className_;
};

using CreateUserObjectBase = std::function<UserObjectBase *(void)>;
class UserObjectBaseLoader {
public:
    static UserObjectBaseLoader &GetInstance(void);
    ~UserObjectBaseLoader() = default;
    /**
     * @brief Registered user-defined serialization class.
     * @param objectName The name of the custom class.
     * @param createFun Function object that creates an instance of a custom class.
     */
    void RegisterUserObject(const std::string &objectName, const CreateUserObjectBase &createFun);

    /**
     * @brief Represents obtaining an instance of an object of a registered serialization class.
     * @param className The name of the custom class {UserObjectBase subClass}.
     *
     * @return Returns an instance of the object, or nullptr on failure.
     */
    UserObjectBase *GetUserObjectByName(const std::string &className);

private:
    std::unordered_map<std::string, CreateUserObjectBase> registerClassList_;
    UserObjectBaseLoader() = default;
    UserObjectBaseLoader(const UserObjectBaseLoader &) = delete;
    UserObjectBaseLoader &operator=(const UserObjectBaseLoader &) = delete;
    UserObjectBaseLoader(UserObjectBaseLoader &&) = delete;
    UserObjectBaseLoader &operator=(UserObjectBaseLoader &&) = delete;
};

/**
 * @brief Register the object's deserialization class, which must be a child UserObjectBase.
 *
 * @param className The name of class.
 */
#define REGISTER_USER_OBJECT_BASE(className)                                                 \
    static __attribute__((constructor)) void RegisterUserMapObject_##className()             \
    {                                                                                        \
        UserObjectBaseLoader::GetInstance().RegisterUserObject(                              \
            #className, []()->UserObjectBase * { return new (std::nothrow)(className); });   \
    }
}  // namespace AAFwk
}  // namespace OHOS
#endif  // OHOS_ABILITY_BASE_USER_OBJECT_BASE_H
