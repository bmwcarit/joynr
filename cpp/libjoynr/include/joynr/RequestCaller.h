/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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
#ifndef REQUESTCALLER_H
#define REQUESTCALLER_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include <QString>

namespace joynr {

class IAttributeListener;

class JOYNR_EXPORT RequestCaller {
public:
    RequestCaller(const QString& interfaceName);
    virtual ~RequestCaller();

    QString getInterfaceName();

    // Get and set the attribute listeners listening on the provider
    virtual void registerAttributeListener(const QString& attributeName, IAttributeListener* attributeListener) = 0;
    virtual void unregisterAttributeListener(const QString& attributeName, IAttributeListener* attributeListener) = 0;

private:
    DISALLOW_COPY_AND_ASSIGN(RequestCaller);
    QString interfaceName;
};


} // namespace joynr
#endif //REQUESTCALLER_H
