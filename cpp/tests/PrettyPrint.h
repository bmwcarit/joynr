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
#ifndef PRETTYPRINT_H_
#define PRETTYPRINT_H_

#include "joynr/JsonSerializer.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/JoynrMessage.h"
#include "joynr/RequestStatus.h"
#include "joynr/RequestStatusCode.h"
#include "joynr/types/Trip.h"

void initPretty(void);

void PrintTo(const joynr::types::GpsLocation& value, ::std::ostream* os);
void PrintTo(const joynr::JoynrMessage& value, ::std::ostream* os);
void PrintTo(const QString& value, ::std::ostream* os);
void PrintTo(const QObject& value, ::std::ostream* os);
void PrintTo(const QVariant& value, ::std::ostream* os);
void PrintTo(const joynr::RequestStatusCode& value, ::std::ostream* os);
void PrintTo(const joynr::RequestStatus& value, ::std::ostream* os);
void PrintTo(const joynr::types::Trip& value, ::std::ostream* os);

#endif /* PRETTYPRINT_H_ */
