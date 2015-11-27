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
#ifndef DBUSSKELETONWRAPPER_H
#define DBUSSKELETONWRAPPER_H

#include "joynr/PrivateCopyAssign.h"

#include "joynr/joynrlogging.h"
#include <QString>
#include <CommonAPI/CommonAPI.h>

#include "joynr/JoynrCommonExport.h"
#include "joynr/TypeUtil.h"

namespace joynr
{

using namespace joynr_logging;

template <class _SkeletonClass, class _CallBackClass>
class JOYNRCOMMON_EXPORT IDbusSkeletonWrapper
{
public:
    IDbusSkeletonWrapper(_CallBackClass& callBack, std::string serviceAddress)
            : // factory(NULL),
              serviceAddress(serviceAddress),
              logger(Logging::getInstance()->getLogger("MSG", "DbusSkeletonWrapper"))
    {
        LOG_INFO(logger, "Registering dbus skeleton on address: " + TypeUtil::toQt(serviceAddress));

        // create the skeleton
        std::shared_ptr<_SkeletonClass> skeleton = std::make_shared<_SkeletonClass>(callBack);

        // register skeleton
        auto runtime = CommonAPI::Runtime::load("DBus");
        bool success = runtime->getServicePublisher()->registerService(
                skeleton, serviceAddress, runtime->createFactory());
        // wait some time so that the service is registered and ready to use on dbus level
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        if (success) {
            LOG_INFO(
                    logger,
                    QString("registering service %1: SUCCESS").arg(TypeUtil::toQt(serviceAddress)));
        } else {
            LOG_FATAL(logger,
                      QString("registering service %1: ERROR").arg(TypeUtil::toQt(serviceAddress)));
        }
    }

    ~IDbusSkeletonWrapper()
    {
        LOG_INFO(logger,
                 "Unregistering dbus skeleton from address: " + TypeUtil::toQt(serviceAddress));

        auto runtime = CommonAPI::Runtime::load("DBus");
        bool success = runtime->getServicePublisher()->unregisterService(serviceAddress);
        // wait some time so that the service is unregistered on dbus level
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        if (success) {
            LOG_INFO(logger,
                     QString("unregistering service %1: SUCCESS")
                             .arg(TypeUtil::toQt(serviceAddress)));
        } else {
            LOG_FATAL(
                    logger,
                    QString("unregistering service %1: ERROR").arg(TypeUtil::toQt(serviceAddress)));
        }
    }

    void logMethodCall(const QString& method, const QString& adapter)
    {
        LOG_INFO(logger,
                 "Call method " + adapter + ":" + TypeUtil::toQt(serviceAddress) + "-> " + method);
    }

private:
    DISALLOW_COPY_AND_ASSIGN(IDbusSkeletonWrapper);
    std::string serviceAddress;
    joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // DBUSSKELETONWRAPPER_H
