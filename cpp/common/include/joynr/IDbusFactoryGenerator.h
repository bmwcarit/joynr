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
#ifndef IDBUSFACTORYGENERATOR_H
#define IDBUSFACTORYGENERATOR_H

#include <QString>
#include <QMap>
#include <CommonAPI/Factory.h>

namespace joynr {

class IDbusFactoryGenerator
{
public:

    static std::shared_ptr<CommonAPI::Factory> getFactoryInstance(bool isProxy, QString instanceId) {
        static QMap<QString, std::shared_ptr<CommonAPI::Factory>> proxyMap;
        static QMap<QString, std::shared_ptr<CommonAPI::Factory>> providerMap;
        if(isProxy) {
            if(proxyMap.count(instanceId) == 1 ) {
                return *(proxyMap.find(instanceId));
            } else {
                auto factory = CommonAPI::Runtime::load("DBus")->createFactory();
                proxyMap.insert(instanceId, factory);
                return factory;
            }
        } else {
            if(providerMap.count(instanceId) == 1 ) {
                return *(providerMap.find(instanceId));
            } else {
                auto factory = CommonAPI::Runtime::load("DBus")->createFactory();
                providerMap.insert(instanceId, factory);
                return factory;
            }
        }
    }
};


} // namespace joynr
#endif // IDBUSFACTORYGENERATOR_H
