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
#ifndef DBUSSKELETONWRAPPER_H
#define DBUSSKELETONWRAPPER_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/joynrlogging.h"
#include <QString>
#include <CommonAPI/Factory.h>

#include "joynr/JoynrCommonExport.h"
#include "joynr/IDbusFactoryGenerator.h"

namespace joynr {

using namespace joynr_logging;

template<class _SkeletonClass, class _CallBackClass>
class JOYNRCOMMON_EXPORT IDbusSkeletonWrapper
{
public:
    IDbusSkeletonWrapper(_CallBackClass& callBack, QString serviceAddress)
        : factory(NULL),
          serviceAddress(serviceAddress),
          logger(Logging::getInstance()->getLogger("MSG", "DbusSkeletonWrapper"))
    {
        LOG_INFO(logger, "Registering dbus skeleton on address: " + serviceAddress);

        // create the skeleton
        std::shared_ptr<_SkeletonClass> skeleton = std::make_shared<_SkeletonClass>(callBack);

        // register skeleton
        factory = IDbusFactoryGenerator::getFactoryInstance(false, serviceAddress);
        bool success = factory->registerService(skeleton, serviceAddress.toStdString());

        if(success) {
            LOG_INFO(logger, "SUCCESS");
        } else {
            LOG_FATAL(logger, "ERROR");
        }
    }

    ~IDbusSkeletonWrapper() {
        LOG_INFO(logger, "Unregistering dbus skeleton from address: " + serviceAddress);

        bool success = factory->unregisterService(serviceAddress.toStdString());
        if(success) {
            LOG_INFO(logger, "SUCCESS");
        } else {
            LOG_FATAL(logger, "ERROR");
        }
    }

    void logMethodCall(const QString& method, const QString& adapter) {
        LOG_INFO(logger, "Call method " + adapter + ":" + serviceAddress + "-> " + method);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(IDbusSkeletonWrapper);
    std::shared_ptr<CommonAPI::Factory> factory;
    QString serviceAddress;
    joynr_logging::Logger* logger;
};


} // namespace joynr
#endif // DBUSSKELETONWRAPPER_H
