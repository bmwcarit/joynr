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
#ifndef INTERFACEREGISTRAR_H
#define INTERFACEREGISTRAR_H

#include "joynr/JoynrExport.h"

#include "joynr/IRequestInterpreter.h"
#include "joynr/RequestCaller.h"

#include <QMutex>
#include <QHash>
#include <string>

namespace joynr
{

/**
  * Registers RequestInterpreters for generated interfaces.
  *
  * An application or library can register its interfaces with this registrar and then
  * libjoynr will use the given instances of the RequestInterpreters
  * to call the RequestCallers that implement the interfaces.
  */
class JOYNR_EXPORT InterfaceRegistrar
{
public:
    /**
      * This class is currently implemented as a singleton
      */
    static InterfaceRegistrar& instance();

    /**
      * Register a request interpreter of type T for the interface with the given name
      */
    template <class T>
    void registerRequestInterpreter(const std::string& interfaceName);

    /**
      * Unregister a request interpreter for the given interface
      */
    void unregisterRequestInterpreter(const std::string& interfaceName);

    /**
      * Get a request interpreter for the given interface
      */
    QSharedPointer<IRequestInterpreter> getRequestInterpreter(const std::string& interfaceName);

    /**
      * Reset the InterfaceRegistrar - for use in tests
      */
    void reset();

private:
    InterfaceRegistrar();
    DISALLOW_COPY_AND_ASSIGN(InterfaceRegistrar);
    static InterfaceRegistrar* registrarInstance;

    // Thread safe hash table of request interpreters
    QHash<QString, QSharedPointer<IRequestInterpreter>> requestInterpreters;
    QMutex requestInterpretersMutex;

    // A count of how many registrations are done for each request interpreter
    // Also protected by requestInterpretersMutex
    QHash<QString, int> requestInterpreterCounts;
};

template <class T>
void InterfaceRegistrar::registerRequestInterpreter(const std::string& interfaceName)
{
    QMutexLocker locker(&requestInterpretersMutex);
    QString qInterfaceName(QString::fromStdString(interfaceName));
    if (!requestInterpreters.contains(qInterfaceName)) {
        requestInterpreters.insert(qInterfaceName, QSharedPointer<IRequestInterpreter>(new T()));
        requestInterpreterCounts.insert(qInterfaceName, 1);
    } else {
        ++requestInterpreterCounts[qInterfaceName];
    }
}

} // namespace joynr
#endif // INTERFACEREGISTRAR_H
