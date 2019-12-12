/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef ABSTRACTBROADCASTLISTENER_H
#define ABSTRACTBROADCASTLISTENER_H

#include "joynr/JoynrExport.h"

namespace joynr
{

class PublicationManager;

class JOYNR_EXPORT AbstractBroadcastListener
{
public:
    /**
     * Create an broadcast listener
     */
    explicit AbstractBroadcastListener(std::weak_ptr<PublicationManager> publicationManager)
            : _publicationManager(std::move(publicationManager))
    {
    }

protected:
    std::weak_ptr<PublicationManager> _publicationManager;
};

} // namespace joynr

#endif // ABSTRACTBROADCASTLISTENER_H
