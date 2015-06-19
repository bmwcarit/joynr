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
#ifndef CLIENTCACHE_H
#define CLIENTCACHE_H

#include <QVariant>
#include <QString>

namespace joynr
{

/*
 * Interface for an attribute cache that can be used by one or several clients.
 */
class IClientCache
{
public:
    virtual ~IClientCache()
    {
    }

    /*
     * Returns the value stored for the attributeId. If none exists, it return an invalid
     * QVariant object that can be tested for by using the isValid() method of QVariant.
     */
    virtual QVariant lookUp(const QString& attributeId) = 0;

    /*
     * Inserts the key (attributeId) and value into the cache.  If the attributeId already
     * has a value, then this overwrites the previous value.
     * Note, this insert does not perform any validation on the value.
     */
    virtual void insert(QString attributeId, QVariant value) = 0;
};

} // namespace joynr
#endif // CLIENTCACHE_H
