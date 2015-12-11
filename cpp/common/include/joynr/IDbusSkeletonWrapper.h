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
#include "joynr/JoynrCommonExport.h"

#include "joynr/joynrlogging.h"
#include "joynr/TypeUtil.h"

#include <string>
#include <CommonAPI/CommonAPI.h>

#include <chrono>

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
        LOG_INFO(
                logger,
                FormatString("Registering dbus skeleton on address: %1").arg(serviceAddress).str());

        // create the skeleton
        std::shared_ptr<_SkeletonClass> skeleton = std::make_shared<_SkeletonClass>(callBack);

        // register skeleton
        auto runtime = CommonAPI::Runtime::load("DBus");
        bool success = runtime->getServicePublisher()->registerService(
                skeleton, serviceAddress, runtime->createFactory());
        // wait some time so that the service is registered and ready to use on dbus level
        std::this_thread::sleep_for(std::chrono::milliseconds(25));

        if (success) {
            LOG_INFO(logger,
                     FormatString("registering service %1: SUCCESS").arg(serviceAddress).str());
        } else {
            LOG_FATAL(logger,
                      FormatString("registering service %1: ERROR").arg(serviceAddress).str());
        }
    }

    ~IDbusSkeletonWrapper()
    {
        LOG_INFO(logger,
                 FormatString("Unregistering dbus skeleton from address: %1")
                         .arg(serviceAddress)
                         .str());

        auto runtime = CommonAPI::Runtime::load("DBus");
        bool success = runtime->getServicePublisher()->unregisterService(serviceAddress);
        // wait some time so that the service is unregistered on dbus level
        ThreadUtil::sleepForMillis(25);

        if (success) {
            LOG_INFO(logger,
                     FormatString("unregistering service %1: SUCCESS").arg(serviceAddress).str());
        } else {
            LOG_FATAL(logger,
                      FormatString("unregistering service %1: ERROR").arg(serviceAddress).str());
        }
    }

    void logMethodCall(const std::string& method, const std::string& adapter)
    {
        LOG_INFO(logger,
                 FormatString("Call method %1:%2-> %3")
                         .arg(adapter.toStdString())
                         .arg(serviceAddress)
                         .arg(method.toStdString())
                         .str());
    }

private:
    DISALLOW_COPY_AND_ASSIGN(IDbusSkeletonWrapper);
    std::string serviceAddress;
    joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // DBUSSKELETONWRAPPER_H
