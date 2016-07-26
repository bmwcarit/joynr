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
#ifndef CLIENTQCACHE_H
#define CLIENTQCACHE_H

#include <string>
#include <mutex>

#include <boost/any.hpp>

#include "joynr/JoynrCommonExport.h"
#include "joynr/IClientCache.h"
#include "joynr/Cache.h"

namespace joynr
{

template <class T>
class CachedValue;

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
    ~ClientQCache() override = default;
    /**
     * Returns the stored object associated with the key 'attributeId',
     * or an empty boost::any if the object is either not present
     */
    boost::any lookUp(const std::string& attributeId) override;
    /**
      * Inserts 'value' into the cache under the key 'attributeId'.
      * The entry is associated with a time stamp guranteed to be valid for 24hrs.
      */
    void insert(std::string attributeId, boost::any value) override;

private:
    Cache<std::string, CachedValue<boost::any>> cache;
    std::mutex mutex;
};

} // namespace joynr

#endif // CLIENTQCACHE_H
