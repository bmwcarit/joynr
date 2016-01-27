/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef READWRITELOCK_H
#define READWRITELOCK_H

#include <shared_mutex>

namespace joynr
{

using ReadWriteLock = std::shared_timed_mutex;
using ReadLocker = std::shared_lock<ReadWriteLock>;
using WriteLocker = std::unique_lock<ReadWriteLock>;

} // namespace joynr

#endif // READWRITELOCK_H
