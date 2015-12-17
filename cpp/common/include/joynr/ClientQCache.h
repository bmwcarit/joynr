/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef ClientQCache_H
#define ClientQCache_H

#include "joynr/JoynrCommonExport.h"

#include "joynr/IClientCache.h"
#include "joynr/CachedValue.h"

#include "joynr/Cache.h"

#include <string>
#include <mutex>
#include "joynr/Variant.h"

namespace joynr
{

/**
 * Implements IClientCache. Stores objects with a key and a timestamp.
 * The timestamp is generated automatically. Access is thread safe.
 * The cache automatically manages the objects that are inserted and
 * deletes them to make room for new objects, if necessary. This is in
 * memory only, no persistence.
 *
 */
class JOYNRCOMMON_EXPORT ClientQCache : public IClientCache
{
public:
    ClientQCache();
    virtual ~ClientQCache() = default;
    /**
     * Returns the stored object associated with the key 'attributeId',
     * or an empty QVariant() if the object is either not present
     */
    Variant lookUp(const std::string& attributeId);
    /**
      * Inserts 'value' into the cache under the key 'attributeId'.
      * The entry is associated with a time stamp guranteed to be valid for 24hrs.
      */
    void insert(std::string attributeId, Variant value);

private:
    /**
      * Time since activation in ms
      */
    int64_t elapsed(int64_t entryTime);
    Cache<std::string, CachedValue<Variant>> cache;
    std::mutex mutex;
};

} // namespace joynr

#endif // ClientQCache_H
