/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_KV_BLOB_H
#define DISTRIBUTED_KV_BLOB_H

#include <string>
#include <vector>
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {

class Blob  {
public:
    /**
     * @brief Constructor.
     */
    API_EXPORT Blob();

    /**
     * @brief Destructor.
     */
    API_EXPORT ~Blob() = default;

    /**
     * @brief Copy constructor for Blob.
     */
    API_EXPORT Blob(const Blob &blob);

    /**
     * @brief Operator =.
     */
    API_EXPORT Blob &operator=(const Blob &blob);

    /**
     * @brief Move constructor for Blob.
     */
    API_EXPORT Blob(Blob &&blob);

    /**
     * @brief Operator =.
     */
    API_EXPORT Blob &operator=(Blob &&blob);

    /**
     * @brief Construct a Blob use std::string.
    */
    API_EXPORT Blob(const std::string &str);

    /**
     * @brief Operator =.
     */
    API_EXPORT Blob &operator=(const std::string &str);

    /**
     * @brief Construct a Blob use char pointer and len.
    */
    API_EXPORT Blob(const char *str, size_t n);

    /**
     * @brief Construct a Blob use char pointer.
    */
    API_EXPORT Blob(const char *str);

    /**
     * @brief Operator =.
     */
    API_EXPORT Blob &operator=(const char *str);

    /**
     * @brief Construct a Blob use std::vector<uint8_t>.
    */
    API_EXPORT Blob(const std::vector<uint8_t> &bytes);

    /**
     * @brief Construct a Blob use std::vector<uint8_t>.
    */
    API_EXPORT Blob(std::vector<uint8_t> &&bytes);

    /**
     * @brief Return a reference to the data of the blob.
    */
    API_EXPORT const std::vector<uint8_t> &Data() const;

    /**
     * @brief Return std::vector<uint8_t> &.
     */
    API_EXPORT operator const std::vector<uint8_t> &() const;

    /**
     * @brief Return std::vector<uint8_t> &&.
     */
    API_EXPORT operator std::vector<uint8_t> &&() noexcept;

    /**
     * @brief Return the length of the referenced data(unit: byte).
     * @return The length of the referenced data.
    */
    API_EXPORT size_t Size() const;

    /**
     * @brief Return the occupied length when write this blob to rawdata.
     * @return The occupied length.
    */
    int RawSize() const;

    /**
     * @brief Indicate the referenced data is zero or not.
     * @return Return true if the length of the referenced data is zero
     *         otherwise return false.
    */
    API_EXPORT bool Empty() const;

    /**
     * @brief  Return the the byte in the referenced data.
     * @param n the number of byte(n < {@link size_t Size()})
    */
    API_EXPORT uint8_t operator[](size_t n) const;

    /**
     * @brief Operator ==.
     */
    API_EXPORT bool operator==(const Blob &) const;

    /**
     * @brief Change this blob to refer to an empty array.
    */
    API_EXPORT void Clear();

    /**
     * @brief Change vector<uint8_t> to std::string.
     * @return The string value.
    */
    API_EXPORT std::string ToString() const;

    /**
     * @brief Comparison. Returns value.
     *
     * <  0 if "*this" <  "blob".
     * == 0 if "*this" == "blob".
     * >  0 if "*this" >  "blob".
    */
    API_EXPORT int Compare(const Blob &blob) const;

    /**
     * @brief Whether the input blob is a prefix of "*this" or not.
     * @param blob The input blob value.
     * @return True if "blob" is a prefix of "*this", otherwise false.
    */
    API_EXPORT bool StartsWith(const Blob &blob) const;

    /**
     * @brief Write blob size and data to memory buffer.
     * @param cursorPtr The pointer of the buffer.
     * @param bufferLeftSize The size of unused buffer.
     * @return Return false if bufferLeftSize not enough, otherwise true.
    */
    bool WriteToBuffer(uint8_t *&cursorPtr, int &bufferLeftSize) const;

    /**
     * @brief Read a blob from memory buffer.
     * @param cursorPtr The pointer of the buffer.
     * @param bufferLeftSize The available buffer.
     * @return Return false if bufferLeftSize not enough, otherwise true.
    */
    bool ReadFromBuffer(const uint8_t *&cursorPtr, int &bufferLeftSize);

private:
    std::vector<uint8_t> blob_;
};
}  // namespace DistributedKv
}  // namespace OHOS

#endif  // DISTRIBUTED_KV_BLOB_H
