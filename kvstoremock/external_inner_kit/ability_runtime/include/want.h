/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_BASE_WANT_H
#define OHOS_ABILITY_BASE_WANT_H

#include <string>
#include <vector>
#include <algorithm>

#include "uri.h"
#include "want_params.h"
#include "element_name.h"
#include "operation.h"
#include "parcel.h"

using Operation = OHOS::AAFwk::Operation;

namespace OHOS {
namespace AAFwk {
class Want final : public Parcelable {
public:
    /**
     * Indicates the grant to perform read operations on the URI.
     */
    static constexpr unsigned int FLAG_AUTH_READ_URI_PERMISSION = 0x00000001;
    /**
     * Indicates the grant to perform write operations on the URI.
     */
    static constexpr unsigned int FLAG_AUTH_WRITE_URI_PERMISSION = 0x00000002;
    /**
     * Returns the result to the source ability.
     */
    static constexpr unsigned int FLAG_ABILITY_FORWARD_RESULT = 0x00000004;
    /**
     * Determines whether an ability on the local device can be migrated to a remote device.
     */
    static constexpr unsigned int FLAG_ABILITY_CONTINUATION = 0x00000008;
    /**
     * Specifies whether a component does not belong to OHOS.
     */
    static constexpr unsigned int FLAG_NOT_OHOS_COMPONENT = 0x00000010;
    /**
     * Specifies whether an ability is started.
     */
    static constexpr unsigned int FLAG_ABILITY_FORM_ENABLED = 0x00000020;
    /**
     * Indicates the grant for possible persisting on the URI.
     */
    static constexpr unsigned int FLAG_AUTH_PERSISTABLE_URI_PERMISSION = 0x00000040;
    /**
     * Returns the result to the source ability slice.
     */
    static constexpr unsigned int FLAG_AUTH_PREFIX_URI_PERMISSION = 0x00000080;
    /**
     * Supports multi-device startup in the distributed scheduling system.
     */
    static constexpr unsigned int FLAG_ABILITYSLICE_MULTI_DEVICE = 0x00000100;
    /**
     * Indicates that an ability using the Service template is started regardless of whether the host application has
     * been started.
     */
    static constexpr unsigned int FLAG_START_FOREGROUND_ABILITY = 0x00000200;

    /**
     * Indicates the continuation is reversible.
     */
    static constexpr unsigned int FLAG_ABILITY_CONTINUATION_REVERSIBLE = 0x00000400;

    /**
     * Install the specified ability if it's not installed.
     */
    static constexpr unsigned int FLAG_INSTALL_ON_DEMAND = 0x00000800;
    /**
     * Indicates the continuation is quick start
     */
    static constexpr unsigned int FLAG_ABILITY_PREPARE_CONTINUATION = 0x00001000;
    /**
     * Support collaborative request lifecycle callback in distributed scheduling systems.
     */
    static constexpr unsigned int FLAG_ABILITY_ON_COLLABORATE = 0x00002000;
    /**
     * Returns the result to the source ability slice.
     */
    static constexpr unsigned int FLAG_ABILITYSLICE_FORWARD_RESULT = 0x04000000;
    /**
     * Install the specifiedi ability with background mode if it's not installed.
     */
    static constexpr unsigned int FLAG_INSTALL_WITH_BACKGROUND_MODE = 0x80000000;
    /**
     * Indicates the operation of clearing other missions.
     */
    static constexpr unsigned int FLAG_ABILITY_CLEAR_MISSION = 0x00008000;
    /**
     * Indicates the operation of creating a task on the historical mission stack.
     */
    static constexpr unsigned int FLAG_ABILITY_NEW_MISSION = 0x10000000;
    /**
     * Indicates that the existing instance of the ability to start will be reused if it is already at the top of
     * the mission stack. Otherwise, a new ability instance will be created.
     */
    static constexpr unsigned int FLAG_ABILITY_MISSION_TOP = 0x20000000;
    /**
     * Indicates that if implicit start ability couldn't match any application, no tip dialog will be pulled up.
     */
    static constexpr unsigned int FLAG_START_WITHOUT_TIPS = 0x40000000;

    /**
     * @description:  Default construcotr of Want class, which is used to initialzie flags and URI.
     * @param None
     * @return None
     */
    Want();

    /**
     * @description: Default deconstructor of Want class
     * @param None
     * @return None
     */
    ~Want();

    /**
     * @description: Copy construcotr of Want class, which is used to initialzie flags, URI, etc.
     * @param want the source instance of Want.
     * @return None
     */
    Want(const Want &want);
    Want &operator=(const Want &want);

    /**
     * @description: Sets a flag in a Want.
     * @param flags Indicates the flag to set.
     * @return Returns this Want object containing the flag.
     */
    Want &SetFlags(unsigned int flags);

    /**
     * @description: Obtains the description of flags in a Want.
     * @return Returns the flag description in the Want.
     */
    unsigned int GetFlags() const;

    /**
     * @description: Adds a flag to a Want.
     * @param flags Indicates the flag to add.
     * @return Returns the Want object with the added flag.
     */
    Want &AddFlags(unsigned int flags);

    /**
     * @description: Removes the description of a flag from a Want.
     * @param flags Indicates the flag to remove.
     * @return Removes the description of a flag from a Want.
     */
    void RemoveFlags(unsigned int flags);

    /**
     * @description: Sets the bundleName and abilityName attributes for this Want object.
     * @param bundleName Indicates the bundleName to set for the operation attribute in the Want.
     * @param abilityName Indicates the abilityName to set for the operation attribute in the Want.
     * @return Returns this Want object that contains the specified bundleName and abilityName attributes.
     */
    Want &SetElementName(const std::string &bundleName, const std::string &abilityName);

    /**
     * @description: Sets the bundleName and abilityName attributes for this Want object.
     * @param deviceId Indicates the deviceId to set for the operation attribute in the Want.
     * @param bundleName Indicates the bundleName to set for the operation attribute in the Want.
     * @param abilityName Indicates the abilityName to set for the operation attribute in the Want.
     * @return Returns this Want object that contains the specified bundleName and abilityName attributes.
     */
    Want &SetElementName(const std::string &deviceId, const std::string &bundleName,
        const std::string &abilityName, const std::string &moduleName = "");

    /**
     * @description: Sets an ElementName object in a Want.
     * @param element Indicates the ElementName description.
     * @return Returns this Want object containing the ElementName
     */
    Want &SetElement(const OHOS::AppExecFwk::ElementName &element);

    /**
     * @description: Obtains the description of the ElementName object in a Want.
     * @return Returns the ElementName description in the Want.
     */
    OHOS::AppExecFwk::ElementName GetElement() const;

    /**
     * @description: Creates a want with its corresponding attributes specified for starting the main ability of an
     * application.
     * @param ElementName  Indicates the ElementName object defining the deviceId, bundleName,
     * and abilityName sub-attributes of the operation attribute in a want.
     * @return Returns the want object used to start the main ability of an application.
     */
    static Want *MakeMainAbility(const OHOS::AppExecFwk::ElementName &elementName);

    /**
     * @description: Creates a Want instance by using a given Uniform Resource Identifier (URI).
     * This method parses the input URI and saves it in a Want object.
     * @param uri Indicates the URI to parse.
     * @return Returns a Want object containing the URI.
     */
    static Want *WantParseUri(const char *uri);

    /**
     * @description: Creates a Want instance by using a given Uniform Resource Identifier (URI).
     * This method parses the input URI and saves it in a Want object.
     * @param uri Indicates the URI to parse.
     * @return Returns a Want object containing the URI.
     */
    static Want *ParseUri(const std::string &uri);

    /**
     * @description: Obtains the description of a URI in a Want.
     * @return Returns the URI description in the Want.
     */
    Uri GetUri() const;

    /**
     * @description: Obtains the string representation of the URI in this Want.
     * @return Returns the string of the URI.
     */
    std::string GetUriString() const;

    /**
     * @description: Sets the description of a URI in a Want.
     * @param uri Indicates the URI description.
     * @return Returns this Want object containing the URI.
     */
    Want &SetUri(const std::string &uri);

    /**
     * @description: Sets the description of a URI in a Want.
     * @param uri Indicates the URI description.
     * @return Returns this Want object containing the URI.
     */
    Want &SetUri(const Uri &uri);

    /**
     * @description: Sets the description of a URI and a type in this Want.
     * @param uri Indicates the URI description.
     * @param type Indicates the type description.
     * @return Returns the Want object containing the URI and the type by setting.
     */
    Want &SetUriAndType(const Uri &uri, const std::string &type);

    /**
     * @description: Converts a Want into a URI string containing a representation of it.
     * @param want Indicates the want description.--Want.
     * @return   Returns an encoding URI string describing the Want object.
     */
    std::string WantToUri(Want &want);

    /**
     * @description: Converts parameter information in a Want into a URI string.
     * @return Returns the URI string.
     */
    std::string ToUri() const;

    /**
     * @description: Formats a specified URI.
     * This method uses the Uri.getLowerCaseScheme() method to format a URI and then saves
     * the formatted URI to this Want object.
     * @param uri Indicates the string of URI to format.
     * @return Returns this Want object that contains the formatted uri attribute.
     */
    Want &FormatUri(const std::string &uri);

    /**
     * @description: Formats a specified URI.
     * This method uses the Uri.getLowerCaseScheme() method to format a URI and then saves
     * the formatted URI to this Want object.
     * @param uri Indicates the URI to format.
     * @return Returns this Want object that contains the formatted URI attribute.
     */
    Want &FormatUri(const Uri &uri);

    /**
     * @description: Obtains the description of an action in a want.
     * @return Returns a Want object that contains the action description.
     */
    std::string GetAction() const;

    /**
     * @description: Sets the description of an action in a want.
     * @param action Indicates the action description to set.
     * @return Returns a Want object that contains the action description.
     */
    Want &SetAction(const std::string &action);

    /**
     * @description: Obtains the name of the specified bundle in a Want.
     * @return Returns the specified bundle name in the Want.
     */
    std::string GetBundle() const;

    /**
     * @description: Sets a bundle name in this Want.
     * If a bundle name is specified in a Want, the Want will match only
     * the abilities in the specified bundle. You cannot use this method and
     * setPicker(ohos.aafwk.content.Want) on the same Want.
     * @param bundleName Indicates the bundle name to set.
     * @return Returns a Want object containing the specified bundle name.
     */
    Want &SetBundle(const std::string &bundleName);

    /**
     * @description: Obtains the description of all entities in a Want
     * @return Returns a set of entities
     */
    const std::vector<std::string> &GetEntities() const;

    /**
     * @description: Adds the description of an entity to a Want
     * @param entity Indicates the entity description to add
     * @return {Want} Returns this Want object containing the entity.
     */
    Want &AddEntity(const std::string &entity);

    /**
     * @description: Removes the description of an entity from a Want
     * @param entity Indicates the entity description to remove.
     * @return void
     */
    void RemoveEntity(const std::string &entity);

    /**
     * @description: Checks whether a Want contains the given entity
     * @param entity Indicates the entity to check
     * @return Returns true if the given entity is contained; returns false otherwise
     */
    bool HasEntity(const std::string &key) const;

    /**
     * @description: Obtains the number of entities in a Want
     * @return Returns the entity quantity
     */
    int CountEntities();

    /**
     * @description: Obtains the description of the URI scheme in this want.
     * @return Returns the URI scheme description in this want.
     */
    const std::string GetScheme() const;

    /**
     * @description: Obtains the description of the type in this Want
     * @return Returns the type description in this Want
     */
    std::string GetType() const;

    /**
     * @description: Sets the description of a type in this Want
     * @param type Indicates the type description
     * @return Returns this Want object containing the type
     */
    Want &SetType(const std::string &type);

    /**
     * @description: Formats a specified MIME type. This method uses
     * the formatMimeType(java.lang.String) method to format a MIME type
     * and then saves the formatted type to this Want object.
     * @param type Indicates the MIME type to format
     * @return Returns this Want object that contains the formatted type attribute
     */
    Want &FormatType(const std::string &type);

    /**
     * @description: Formats a specified URI and MIME type.
     * This method works in the same way as formatUri(ohos.utils.net.URI)
     * and formatType(java.lang.String).
     * @param uri Indicates the URI to format.
     * @param type Indicates the MIME type to format.
     * @return Returns this Want object that contains the formatted URI and type attributes.
     */
    Want &FormatUriAndType(const Uri &uri, const std::string &type);

    /**
     * @description: This method formats data of a specified MIME type
     * by removing spaces from the data and converting the data into
     * lowercase letters. You can use this method to normalize
     * the external data used to create Want information.
     * @param type Indicates the MIME type to format
     * @return Returns this Want object that contains the formatted type attribute
     */
    static std::string FormatMimeType(const std::string &mimeType);

    /**
     * @description: clear the specific want object.
     * @param want Indicates the want to clear
     */
    static void ClearWant(Want *want);

    /**
     * @description: Obtains the description of the WantParams object in a Want
     * @return Returns the WantParams description in the Want
     */
    const WantParams &GetParams() const;

    /**
     * @description: Sets a wantParams object in a want.
     * @param wantParams  Indicates the wantParams description.
     * @return Returns this want object containing the wantParams.
     */
    Want &SetParams(const WantParams &wantParams);

    /**
     * @description: Obtains a bool-type value matching the given key.
     * @param key   Indicates the key of WantParams.
     * @param defaultValue  Indicates the default bool-type value.
     * @return Returns the bool-type value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    bool GetBoolParam(const std::string &key, bool defaultValue) const;

    /**
     * @description: Obtains a bool-type array matching the given key.
     * @param key   Indicates the key of WantParams.
     * @return Returns the bool-type array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<bool> GetBoolArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the IRemoteObject type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the IRemoteObject value of the parameter.
     * @return Returns this want object containing the parameter value.
     */
    Want& SetParam(const std::string& key, const sptr<IRemoteObject>& remoteObject);

    /**
     * @description: Obtains a IRemoteObject-type value matching the given key.
     * @param key   Indicates the key of WantParams.
     * @param defaultValue  Indicates the default IRemoteObject-type value.
     * @return Returns the IRemoteObject-type value of the parameter matching the given key;
     * returns the nullptr if the key does not exist.
     */
    sptr<IRemoteObject> GetRemoteObject(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the boolean type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the boolean value of the parameter.
     * @return Returns this want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, bool value);

    /**
     * @description: Sets a parameter value of the boolean array type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the boolean array of the parameter.
     * @return Returns this want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<bool> &value);

    /**
     * @description: Obtains a byte-type value matching the given key.
     * @param key   Indicates the key of WantParams.
     * @param defaultValue  Indicates the default byte-type value.
     * @return Returns the byte-type value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    byte GetByteParam(const std::string &key, byte defaultValue) const;

    /**
     * @description: Obtains a byte-type array matching the given key.
     * @param key   Indicates the key of WantParams.
     * @return Returns the byte-type array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<byte> GetByteArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the byte type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, byte value);

    /**
     * @description: Sets a parameter value of the byte array type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the byte array of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<byte> &value);

    /**
     * @description: Obtains a char value matching the given key.
     * @param key   Indicates the key of wnatParams.
     * @param value Indicates the default char value.
     * @return Returns the char value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    zchar GetCharParam(const std::string &key, zchar defaultValue) const;

    /**
     * @description: Obtains a char array matching the given key.
     * @param key   Indicates the key of wantParams.
     * @return Returns the char array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<zchar> GetCharArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the char type.
     * @param key   Indicates the key of wantParams.
     * @param value Indicates the char value of the parameter.
     * @return Returns this want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, zchar value);

    /**
     * @description: Sets a parameter value of the char array type.
     * @param key   Indicates the key of wantParams.
     * @param value Indicates the char array of the parameter.
     * @return Returns this want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<zchar> &value);

    /**
     * @description: Obtains an int value matching the given key.
     * @param key   Indicates the key of wantParams.
     * @param value Indicates the default int value.
     * @return Returns the int value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    int GetIntParam(const std::string &key, int defaultValue) const;

    /**
     * @description: Obtains an int array matching the given key.
     * @param key   Indicates the key of wantParams.
     * @return Returns the int array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<int> GetIntArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the int type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the int value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, int value);

    /**
     * @description: Sets a parameter value of the int array type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the int array of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<int> &value);

    /**
     * @description: Obtains a double value matching the given key.
     * @param key   Indicates the key of wantParams.
     * @param defaultValue  Indicates the default double value.
     * @return Returns the double value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    double GetDoubleParam(const std::string &key, double defaultValue) const;

    /**
     * @description: Obtains a double array matching the given key.
     * @param key   Indicates the key of WantParams.
     * @return Returns the double array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<double> GetDoubleArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the double type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the int value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, double value);

    /**
     * @description: Sets a parameter value of the double array type.
     * @param key   Indicates the key matching the parameter.
     * @param value Indicates the double array of the parameter.
     * @return Returns this want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<double> &value);

    /**
     * @description: Obtains a float value matching the given key.
     * @param key   Indicates the key of wnatParams.
     * @param value Indicates the default float value.
     * @return Returns the float value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    float GetFloatParam(const std::string &key, float defaultValue) const;

    /**
     * @description: Obtains a float array matching the given key.
     * @param key Indicates the key of WantParams.
     * @return Obtains a float array matching the given key.
     */
    std::vector<float> GetFloatArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the float type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, float value);

    /**
     * @description: Sets a parameter value of the float array type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<float> &value);

    /**
     * @description: Obtains a long value matching the given key.
     * @param key Indicates the key of wantParams.
     * @param value Indicates the default long value.
     * @return Returns the long value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    long GetLongParam(const std::string &key, long defaultValue) const;

    /**
     * @description: Obtains a long array matching the given key.
     * @param key Indicates the key of wantParams.
     * @return Returns the long array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<long> GetLongArrayParam(const std::string &key) const;

    Want &SetParam(const std::string &key, long long value);

    /**
     * @description: Sets a parameter value of the long type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, long value);

    /**
     * @description: Sets a parameter value of the long array type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<long> &value);

    /**
     * @description: a short value matching the given key.
     * @param key Indicates the key of wantParams.
     * @param defaultValue Indicates the default short value.
     * @return Returns the short value of the parameter matching the given key;
     * returns the default value if the key does not exist.
     */
    short GetShortParam(const std::string &key, short defaultValue) const;

    /**
     * @description: Obtains a short array matching the given key.
     * @param key Indicates the key of wantParams.
     * @return Returns the short array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<short> GetShortArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the short type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, short value);

    /**
     * @description: Sets a parameter value of the short array type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<short> &value);

    /**
     * @description: Obtains a string value matching the given key.
     * @param key Indicates the key of wantParams.
     * @return Returns the string value of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::string GetStringParam(const std::string &key) const;

    /**
     * @description: Obtains a string array matching the given key.
     * @param key Indicates the key of wantParams.
     * @return Returns the string array of the parameter matching the given key;
     * returns null if the key does not exist.
     */
    std::vector<std::string> GetStringArrayParam(const std::string &key) const;

    /**
     * @description: Sets a parameter value of the string type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::string &value);

    /**
     * @description: Sets a parameter value of the string array type.
     * @param key Indicates the key matching the parameter.
     * @param value Indicates the byte-type value of the parameter.
     * @return Returns this Want object containing the parameter value.
     */
    Want &SetParam(const std::string &key, const std::vector<std::string> &value);

    /**
     * @description: Checks whether a Want contains the parameter matching a given key.
     * @param key Indicates the key.
     * @return Returns true if the Want contains the parameter; returns false otherwise.
     */
    bool HasParameter(const std::string &key) const;

    /**
     * @description: Replaces parameters in this Want object with those in the given WantParams object.
     * @param wantParams Indicates the WantParams object containing the new parameters.
     * @return Returns this Want object containing the new parameters.
     */
    Want *ReplaceParams(WantParams &wantParams);

    /**
     * @description: Replaces parameters in this Want object with those in the given Want object.
     * @param want Indicates the Want object containing the new parameters.
     * @return Returns this Want object containing the new parameters.
     */
    Want *ReplaceParams(Want &want);

    /**
     * @description: Removes the parameter matching the given key.
     * @param key Indicates the key matching the parameter to be removed.
     */
    void RemoveParam(const std::string &key);

    /**
     * @description: Gets the description of an operation in a Want.
     * @return Returns the operation included in this Want.
     */
    Operation GetOperation() const;

    /**
     * @description: Sets the description of an operation in a Want.
     * @param operation Indicates the operation description.
     */
    void SetOperation(const OHOS::AAFwk::Operation &operation);

    /**
     * @description: Sets the description of an operation in a Want.
     * @param want Indicates the Want object to compare.
     * @return Returns true if the operation components of the two objects are equal; returns false otherwise.
     */
    bool OperationEquals(const Want &want);

    bool IsEquals(const Want &want);

    /**
     * @description: Creates a Want object that contains only the operation component of this Want.
     * @return Returns the created Want object.
     */
    Want *CloneOperation();

    /**
     * @description: Marshals a Want into a Parcel.
     * Fields in the Want are marshalled separately. If any field fails to be marshalled, false is returned.
     * @param parcel Indicates the Parcel object for marshalling.
     * @return Returns true if the marshalling is successful; returns false otherwise.
     */
    virtual bool Marshalling(Parcel &parcel) const;

    /**
     * @description: Unmarshals a Want from a Parcel.
     * Fields in the Want are unmarshalled separately. If any field fails to be unmarshalled, false is returned.
     * @param parcel Indicates the Parcel object for unmarshalling.
     * @return Returns true if the unmarshalling is successful; returns false otherwise.
     */
    static Want *Unmarshalling(Parcel &parcel);

    void DumpInfo(int level) const;

    std::string ToString() const;

    static Want *FromString(std::string &string);

    /**
    * @description: Sets a device id in a Want.
    * @param deviceId Indicates the device id to set.
    * @return Returns this Want object containing the flag.
    */
    Want &SetDeviceId(const std::string &deviceId);

    std::string GetDeviceId() const;

    /**
     * @description: Sets an ModuleName object in a Want.
     * @param moduleName Indicates the ModuleName description.
     * @return Returns this Want object containing the ModuleName.
     */
    Want &SetModuleName(const std::string &moduleName);

    /**
     * @description: Obtains the description of the ModuleName object in a Want.
     * @return Returns the ModuleName description in the Want.
     */
    std::string GetModuleName() const;

    void CloseAllFd();

    void RemoveAllFd();

    void DupAllFd();

    void SetEntities(const std::vector<std::string> &entities);
    static int32_t Flags_ConvertEts2Native(const int32_t index);
    static int32_t Flags_ConvertNative2Ets(const int32_t nativeValue);
    static std::string Action_ConvertEts2Native(const int32_t index);
    static int32_t Action_ConvertNative2Ets(const std::string nativeValue);

public:
    // action definition
    static const std::string ACTION_PLAY;
    static const std::string ACTION_HOME;

    // entity definition
    static const std::string ENTITY_HOME;
    static const std::string ENTITY_VIDEO;
    static const std::string FLAG_HOME_INTENT_FROM_SYSTEM;
    static const std::string ENTITY_MUSIC;
    static const std::string ENTITY_EMAIL;
    static const std::string ENTITY_CONTACTS;
    static const std::string ENTITY_MAPS;
    static const std::string ENTITY_BROWSER;
    static const std::string ENTITY_CALENDAR;
    static const std::string ENTITY_MESSAGING;
    static const std::string ENTITY_FILES;
    static const std::string ENTITY_GALLERY;

    static constexpr int HEX_STRING_BUF_LEN = 36;
    static constexpr int HEX_STRING_LEN = 10;

    // reserved param definition
    static const std::string PARAM_RESV_WINDOW_MODE;
    static const std::string PARAM_RESV_DISPLAY_ID;
    static const std::string PARAM_RESV_WITH_ANIMATION;
    static const std::string PARAM_RESV_WINDOW_FOCUSED;
    static const std::string PARAM_RESV_WINDOW_LEFT;
    static const std::string PARAM_RESV_WINDOW_TOP;
    static const std::string PARAM_RESV_WINDOW_WIDTH;
    static const std::string PARAM_RESV_WINDOW_HEIGHT;
    static const std::string PARAM_RESV_MIN_WINDOW_WIDTH;
    static const std::string PARAM_RESV_MIN_WINDOW_HEIGHT;
    static const std::string PARAM_RESV_MAX_WINDOW_WIDTH;
    static const std::string PARAM_RESV_MAX_WINDOW_HEIGHT;
    static const std::string PARAM_RESV_CALLER_TOKEN;
    static const std::string PARAM_RESV_CALLER_BUNDLE_NAME;
    static const std::string PARAM_RESV_CALLER_ABILITY_NAME;
    static const std::string PARAM_RESV_CALLER_NATIVE_NAME;
    static const std::string PARAM_RESV_CALLER_APP_ID;
    static const std::string PARAM_RESV_CALLER_APP_IDENTIFIER;
    static const std::string PARAM_RESV_CALLER_UID;
    static const std::string PARAM_RESV_CALLER_PID;
    static const std::string PARAM_RESV_CALLER_APP_CLONE_INDEX;
    static const std::string PARAM_RESV_FOR_RESULT;
    static const std::string PARAM_RESV_CALL_TO_FOREGROUND;
    static const std::string PARAM_RESV_START_RECENT;
    static const std::string PARAM_RESV_REQUEST_PROC_CODE;
    static const std::string PARAM_RESV_REQUEST_TOKEN_CODE;
    static const std::string PARAM_RESV_ABILITY_INFO_CALLBACK;
    static const std::string PARAM_RESV_START_TIME;
    static const std::string PARAM_ABILITY_ACQUIRE_SHARE_DATA;
    static const std::string PARAM_ABILITY_RECOVERY_RESTART;
    static const std::string PARAM_ABILITY_URITYPES;
    static const std::string PARAM_ABILITY_APPINFOS;
    static const std::string PARAM_ASSERT_FAULT_SESSION_ID;
    static const std::string PARAM_STRING_TRANS_FORMAT_UTF8;
    // module name string
    static const std::string PARAM_MODULE_NAME;

    // parameter key
    static const std::string PARAM_BACK_TO_OTHER_MISSION_STACK;
    static const std::string PARM_LAUNCH_REASON_MESSAGE;

    // application auto startup launch reason
    static const std::string PARAM_APP_AUTO_STARTUP_LAUNCH_REASON;

    // app clone index
    static const std::string PARAM_APP_CLONE_INDEX_KEY;
    static const std::string APP_INSTANCE_KEY;
    static const std::string CREATE_APP_INSTANCE_KEY;

    static const std::string PARAM_ATOMIC_SERVICE_PAGE_PATH;
    static const std::string PARAM_ATOMIC_SERVICE_ROUTER_NAME;
    static const std::string PARAM_ATOMIC_SERVICE_PAGE_SOURCE_FILE;
    static const std::string PARAM_ATOMIC_SERVICE_BUILD_FUNCTION;
    static const std::string PARAM_ATOMIC_SERVICE_SUB_PACKAGE_NAME;

    static const std::string PARAMS_REAL_CALLER_KEY;
    static const std::string DESTINATION_PLUGIN_ABILITY;

    // keep-alive
    static const std::string PARAM_APP_KEEP_ALIVE_ENABLED;

    static const std::string START_ABILITY_WITH_WAIT_OBSERVER_ID_KEY;
    // unified data key
    static const std::string PARAM_ABILITY_UNIFIED_DATA_KEY;
    static const std::string ATOMIC_SERVICE_SHARE_ROUTER;
    // expansion flag
    static const std::string PARAM_WANT_EXPANSION_TAG;
    static const int32_t PARAM_WANT_CAPACITY_EXPANSION;

    static const std::string UI_EXTENSION_ROOT_TOKEN;
private:
    WantParams parameters_;
    Operation operation_;

    static const std::string OCT_EQUALSTO;
    static const std::string OCT_SEMICOLON;
    static const std::string MIME_TYPE;
    static const std::string WANT_HEADER;
    static const std::string WANT_END;

    // no object in parcel
    static constexpr int VALUE_NULL = -1;
    // object exist in parcel
    static constexpr int VALUE_OBJECT = 1;

private:
    static bool ParseFlag(const std::string &content, Want &want);
    static std::string Decode(const std::string &str);
    static std::string Encode(const std::string &str);
    static bool ParseContent(const std::string &content, std::string &prop, std::string &value);
    static bool ParseUriInternal(const std::string &content, OHOS::AppExecFwk::ElementName &element, Want &want);
    static bool CheckUri(const std::string &uri);
    bool ReadFromParcel(Parcel &parcel);
    static bool CheckAndSetParameters(Want &want, const std::string &key, std::string &prop, const std::string &value);
    Uri GetLowerCaseScheme(const Uri &uri);
    void ToUriStringInner(std::string &uriString) const;
    void UriStringAppendParam(std::string &uriString) const;
    bool WriteUri(Parcel &parcel) const;
    bool WriteEntities(Parcel &parcel) const;
    bool WriteElement(Parcel &parcel) const;
    bool WriteParameters(Parcel &parcel) const;
    bool ReadUri(Parcel &parcel);
    bool ReadEntities(Parcel &parcel);
    bool ReadElement(Parcel &parcel);
    bool ReadParameters(Parcel &parcel);
    /*  enum Flags {
            FLAG_AUTH_READ_URI_PERMISSION = 0x00000001,
            FLAG_AUTH_WRITE_URI_PERMISSION = 0x00000002,
            FLAG_AUTH_PERSISTABLE_URI_PERMISSION = 0x00000040,
            FLAG_INSTALL_ON_DEMAND = 0x00000800,
            FLAG_START_WITHOUT_TIPS = 0x40000000
       }
    */
    static constexpr std::array<int, 5> FlagsArray_ = {0x00000001, 0x00000002, 0x00000040, 0x00000800, 0x40000000};
    /*  enum Action {
            ACTION_HOME = 'ohos.want.action.home',
            ACTION_DIAL = 'ohos.want.action.dial',
            ACTION_SEARCH = 'ohos.want.action.search',
            ACTION_WIRELESS_SETTINGS = 'ohos.settings.wireless',
            ACTION_MANAGE_APPLICATIONS_SETTINGS = 'ohos.settings.manage.applications',
            ACTION_APPLICATION_DETAILS_SETTINGS = 'ohos.settings.application.details',
            ACTION_SET_ALARM = 'ohos.want.action.setAlarm',
            ACTION_SHOW_ALARMS = 'ohos.want.action.showAlarms',
            ACTION_SNOOZE_ALARM = 'ohos.want.action.snoozeAlarm',
            ACTION_DISMISS_ALARM = 'ohos.want.action.dismissAlarm',
            ACTION_DISMISS_TIMER = 'ohos.want.action.dismissTimer',
            ACTION_SEND_SMS = 'ohos.want.action.sendSms',
            ACTION_CHOOSE = 'ohos.want.action.choose',
            ACTION_IMAGE_CAPTURE = 'ohos.want.action.imageCapture',
            ACTION_VIDEO_CAPTURE = 'ohos.want.action.videoCapture',
            ACTION_SELECT = 'ohos.want.action.select',
            ACTION_SEND_DATA = 'ohos.want.action.sendData',
            ACTION_SEND_MULTIPLE_DATA = 'ohos.want.action.sendMultipleData',
            ACTION_SCAN_MEDIA_FILE = 'ohos.want.action.scanMediaFile',
            ACTION_VIEW_DATA = 'ohos.want.action.viewData',
            ACTION_EDIT_DATA = 'ohos.want.action.editData',
            INTENT_PARAMS_INTENT = 'ability.want.params.INTENT',
            INTENT_PARAMS_TITLE = 'ability.want.params.TITLE',
            ACTION_FILE_SELECT = 'ohos.action.fileSelect',
            PARAMS_STREAM = 'ability.params.stream',
            ACTION_APP_ACCOUNT_OAUTH = 'ohos.account.appAccount.action.oauth'
        }
    */
    static constexpr std::array<const char *, 26> ActionArray_ = {
        "ohos.want.action.home",
        "ohos.want.action.dial",
        "ohos.want.action.search",
        "ohos.settings.wireless",
        "ohos.settings.manage.applications",
        "ohos.settings.application.details",
        "ohos.want.action.setAlarm",
        "ohos.want.action.showAlarms",
        "ohos.want.action.snoozeAlarm",
        "ohos.want.action.dismissAlarm",
        "ohos.want.action.dismissTimer",
        "ohos.want.action.sendSms",
        "ohos.want.action.choose",
        "ohos.want.action.imageCapture",
        "ohos.want.action.videoCapture",
        "ohos.want.action.select",
        "ohos.want.action.sendData",
        "ohos.want.action.sendMultipleData",
        "ohos.want.action.scanMediaFile",
        "ohos.want.action.viewData",
        "ohos.want.action.editData",
        "ability.want.params.INTENT",
        "ability.want.params.TITLE",
        "ohos.action.fileSelect",
        "ability.params.stream",
        "ohos.account.appAccount.action.oauth"
    };
};
}  // namespace AAFwk
}  // namespace OHOS

#endif  // OHOS_ABILITY_BASE_WANT_H
