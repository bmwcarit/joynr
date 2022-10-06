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
#ifndef ITIMEOUTLISTENER_H
#define ITIMEOUTLISTENER_H

namespace joynr
{

/**
 * @brief This interface should be used for objects stored in the Directory that
 * wish to be notified when the time to live expires.
 */
class ITimeoutListener
{
public:
    /**
     * @brief This method will be called by the directory when
     * a time out occurs.
     * After this method is called, the directory will remove
     * this object from its directory.
     */
    virtual void timeOut() = 0;
    virtual ~ITimeoutListener() = default;
};

} // namespace joynr
#endif // ITIMEOUTLISTENER_H
