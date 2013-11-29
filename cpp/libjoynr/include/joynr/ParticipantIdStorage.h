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
#ifndef PARTICIPANTIDSTORAGE_H
#define PARTICIPANTIDSTORAGE_H

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include <QString>

namespace joynr {

/**
 * Creates and persists participant ids.
 *
 * This class is thread safe.
 */
class JOYNR_EXPORT ParticipantIdStorage
{
public:
    /**
     * Persist participant ids using the given file
     */
    ParticipantIdStorage(const QString& filename);
    virtual ~ParticipantIdStorage() {}

    /**
     * Get a provider participant id
     */
    virtual QString getProviderParticipantId(const QString& domain,
                                     const QString& interfaceName,
                                     const QString& authenticationToken);

    /**
     * Get a provider participant id or use a default
     */
    virtual QString getProviderParticipantId(const QString& domain,
                                     const QString& interfaceName,
                                     const QString& authenticationToken,
                                     const QString& defaultValue);

private:
    DISALLOW_COPY_AND_ASSIGN(ParticipantIdStorage);
    QString filename;
};


} // namespace joynr
#endif // PARTICIPANTIDSTORAGE_H
