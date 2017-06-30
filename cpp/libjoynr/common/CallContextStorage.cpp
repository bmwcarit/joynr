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

#include "joynr/CallContextStorage.h"

namespace joynr
{

thread_local CallContext CallContextStorage::callContext;

void CallContextStorage::set(const CallContext& callContext)
{
    CallContextStorage::callContext = callContext;
}

void CallContextStorage::set(CallContext&& callContext)
{
    CallContextStorage::callContext = std::move(callContext);
}

const CallContext& CallContextStorage::get()
{
    return callContext;
}

void CallContextStorage::invalidate()
{
    callContext.invalidate();
}

} // namespace joynr
