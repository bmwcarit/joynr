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
#include <QtDebug>

#include "joynr/JsonSerializer.h"
#include "joynr/types/GpsLocation.h"
#include "joynr/types/Trip.h"
#include "joynr/JoynrMessage.h"
#include <gtest/gtest.h>
#include "PrettyPrint.h"

using namespace joynr;

 void PrintTo(const types::GpsLocation& value, ::std::ostream* os) {
  *os << JsonSerializer::serialize(value).constData() << std::endl;
}

 void PrintTo(const types::Trip& value, ::std::ostream* os) {
  *os << JsonSerializer::serialize(value).constData() << std::endl;
}


 void PrintTo(const JoynrMessage& value, ::std::ostream* os) {
  *os << JsonSerializer::serialize(value).constData() << std::endl;
}

 void PrintTo(const QString& value, ::std::ostream* os) {
     *os << value.toStdString() << std::endl;
}

 void PrintTo(const RequestStatusCode& value, ::std::ostream* os) {
     *os << value.toString().toStdString() << std::endl;
 }

 void PrintTo(const RequestStatus& value, ::std::ostream* os) {
     *os << value.toString().toStdString() << std::endl;
 }


 void initPretty(void) {
 EXPECT_TRUE(false) << ::testing::PrintToString(QString("hello"));
 EXPECT_TRUE(false) << ::testing::PrintToString(JoynrMessage());
 EXPECT_TRUE(false) << ::testing::PrintToString(types::GpsLocation(types::GpsFixEnum::Mode2D, 11,22,33,0, 0, 0, 0, 0, 44));
// EXPECT_TRUE(false) << ::testing::PrintToString(Trip("tripName", GpsLocation(GpsFixEnum::Mode2D,1,2,3,0, 0, 0, 0, 0, 4), QList<types::GpsLocation>()));
}

