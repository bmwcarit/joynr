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
#include <gtest/gtest.h>
#include "PrettyPrint.h"
#include <QtDebug>

#include "joynr/JsonSerializer.h"
#include "joynr/types/Localisation/QtGpsLocation.h"
#include "joynr/types/Localisation/QtTrip.h"
#include "joynr/JoynrMessage.h"
#include "joynr/types/TestTypes/QtTStruct.h"
#include "joynr/types/QtDiscoveryEntry.h"
#include "joynr/system/RoutingTypes/QtWebSocketAddress.h"

using namespace joynr;

namespace joynr {
namespace types {
    void PrintTo(const joynr::types::TestTypes::QtTStruct& value, ::std::ostream* os) {
        *os << joynr::JsonSerializer::serialize(value).constData();
    }

    void PrintTo(const joynr::types::Localisation::QtGpsLocation& value, ::std::ostream* os) {
        *os << joynr::JsonSerializer::serialize(value).constData();
    }

    void PrintTo(const joynr::types::Localisation::QtTrip& value, ::std::ostream* os) {
        *os << JsonSerializer::serialize(value).constData() << std::endl;
    }
}
namespace types {
    void PrintTo(const joynr::types::QtDiscoveryEntry& value, ::std::ostream* os) {
        *os << JsonSerializer::serialize(value).constData() << std::endl;
    }
}
namespace system {
    void PrintTo(const joynr::system::RoutingTypes::QtWebSocketAddress& value, ::std::ostream* os) {
        *os << JsonSerializer::serialize(value).constData() << std::endl;
    }
}
}

void PrintTo(const QString& value, ::std::ostream* os) {
     *os << value.toStdString();
}

void PrintTo(const QChar& value, ::std::ostream* os) {
     *os << value.toLatin1();
}

void PrintTo(const QByteArray& value, std::ostream* os)
{
    *os << value.data();
}

void PrintTo(const QUrl& value, std::ostream* os)
{
    *os << value.toString().toStdString();
}

 void PrintTo(const RequestStatusCode& value, ::std::ostream* os) {
     *os << value.toString() << std::endl;
 }

 void PrintTo(const RequestStatus& value, ::std::ostream* os) {
     *os << value.toString() << std::endl;
 }


// void initPretty(void) {
// EXPECT_TRUE(false) << ::testing::PrintToString(QString("hello"));
// EXPECT_TRUE(false) << ::testing::PrintToString(JoynrMessage());
// EXPECT_TRUE(false) << ::testing::PrintToString(types::QtGpsLocation(1.1, 2.2, 3.3, types::QtGpsFixEnum::MODE2D, 0.0, 0.0, 0.0, 0.0, 444, 444, 444));
//// EXPECT_TRUE(false) << ::testing::PrintToString(QtTrip("tripName", QtGpsLocation(QtGpsFixEnum::MODE2D,1,2,3,0, 0, 0, 0, 0, 4), QList<types::QtGpsLocation>()));
// }

