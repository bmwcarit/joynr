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
#include "joynr/ProxyQos.h"

namespace joynr {

const qint64 ProxyQos::NO_CACHE_FRESHNESS_REQ = -1;
const qint64 ProxyQos::DONT_USE_CACHE= 0;


ProxyQos::ProxyQos():
    reqCacheDataFreshness_ms(-1)
{
}

ProxyQos::ProxyQos(const qint64& reqCacheDataFreshness_ms):
    reqCacheDataFreshness_ms(reqCacheDataFreshness_ms)
{
}

void ProxyQos::setReqCacheDataFreshness_ms(const qint64 &reqCacheDataFreshness_ms)
{
    this->reqCacheDataFreshness_ms = reqCacheDataFreshness_ms;
}

qint64 ProxyQos::getReqCacheDataFreshness_ms()
{
    return reqCacheDataFreshness_ms;
}

} // namespace joynr
