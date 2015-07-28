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
#include "joynr/types/QtGpsPosition.h"
#include "joynr/types/QtGpsPositionExtended.h"
#include "joynr/types/QtGpsLocation.h"
#include "joynr/types/QtGpsFixEnum.h"
#include "joynr/types/QtTrip.h"
#include "joynr/JsonSerializer.h"
#include "joynr/joynrlogging.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/Util.h"
#include <QList>

#include "joynr/types/QtTStruct.h"
#include "joynr/types/QtTStructExtended.h"

using namespace joynr;
using namespace joynr_logging;

class JsonSerializerPolymorphismTest : public testing::Test {
public:
    JsonSerializerPolymorphismTest() :
        logger(joynr_logging::Logging::getInstance()->getLogger("TST", "JsonSerializerPolymorphismTest"))
    {

    }

protected:
    joynr_logging::Logger* logger;
};

TEST_F(JsonSerializerPolymorphismTest, deserializeSuperTypeFromDerivedType) {
    joynr::types::QtTStruct expectedSuper;
    expectedSuper.setTDouble(1.1);
    expectedSuper.setTInt64(64);
    expectedSuper.setTString("test string");

    joynr::types::QtTStructExtended derived;
    derived.setTDouble(1.1);
    derived.setTInt64(64);
    derived.setTString("test string");
    derived.setTEnum(joynr::types::QtTEnum::TLITERALA);
    derived.setTInt32(32);

    QByteArray serializedDerived = JsonSerializer::serialize(derived);

    joynr::types::QtTStruct* deserializedSuper =
            JsonSerializer::deserialize<joynr::types::QtTStruct>(serializedDerived);
    EXPECT_EQ(expectedSuper, *deserializedSuper);

    // this means the deserialized super is actually a derived
    joynr::types::QtTStructExtended* castedDerived = qobject_cast<joynr::types::QtTStructExtended*>(deserializedSuper);
    EXPECT_EQ(derived, *castedDerived);

    delete deserializedSuper;
}

// This test is disabled:
// This test simlates our current behavior of handing over method parameters
// copy-by-value. Calling the copy constructor of the super type (cf. [1] below)
// causes all data of derived types to be lost (when passing derived types as
// parameters to methods).
TEST_F(JsonSerializerPolymorphismTest, DISABLED_serializeDerivedTypeFromSuperType) {
    joynr::types::QtTStruct expectedSuper;
    expectedSuper.setTDouble(1.1);
    expectedSuper.setTInt64(64);
    expectedSuper.setTString("test string");

    joynr::types::QtTStructExtended derived;
    derived.setTDouble(1.1);
    derived.setTInt64(64);
    derived.setTString("test string");
    derived.setTEnum(joynr::types::QtTEnum::TLITERALA);
    derived.setTInt32(32);

    // [1] calls the copy constructor of the super type, all data of derived
    // type is lost
    joynr::types::QtTStruct super(derived);

    QByteArray serializedSuper = JsonSerializer::serialize(super);

    LOG_DEBUG(logger, QString(serializedSuper));

    joynr::types::QtTStruct* deserializedSuper =
            JsonSerializer::deserialize<joynr::types::QtTStruct>(serializedSuper);
    EXPECT_EQ(expectedSuper, *deserializedSuper);

    // this means the deserialized super is actually a derived
    joynr::types::QtTStructExtended* castedDerived = qobject_cast<joynr::types::QtTStructExtended*>(deserializedSuper);
    EXPECT_TRUE(castedDerived != NULL); // cast successfull?
    if(castedDerived != NULL) {
        // causes seg-fault if cast is not successfull
        EXPECT_EQ(derived, *castedDerived);
    }

    delete deserializedSuper;
}

TEST_F(JsonSerializerPolymorphismTest, serializeGpsLocationListWithLocationInside) {
    qRegisterMetaType<joynr::types::QtGpsPosition>("joynr::types::QtGpsPosition");
    qRegisterMetaType<joynr__types__QtGpsPosition>("joynr__types__GpsPosition");
    qRegisterMetaType<joynr::types::QtGpsPositionExtended>("joynr::types::QtGpsPositionExtended");
    qRegisterMetaType<joynr__types__QtGpsPositionExtended>("joynr__types__GpsPositionExtended");
    qRegisterMetaType<joynr::types::QtGpsLocation>("joynr::types::QtGpsLocation");
    qRegisterMetaType<joynr__types__QtGpsLocation>("joynr__types__GpsLocation");

    joynr::types::QtGpsPosition gpsPosition;
    gpsPosition.setLongitude(1.1);
    gpsPosition.setLatitude(2.1);

    joynr::types::QtGpsPositionExtended gpsPositionExt;
    gpsPositionExt.setLongitude(1.2);
    gpsPositionExt.setLatitude(2.2);
    gpsPositionExt.setAltitude(3.2);
    gpsPositionExt.setGpsFix(joynr::types::QtGpsFixEnum::MODE2D);
    gpsPositionExt.setHeading(4.2);
    gpsPositionExt.setQuality(5.2);

    joynr::types::QtGpsLocation gpsLocation;
    gpsLocation.setLongitude(1.3);
    gpsLocation.setLatitude(2.3);
    gpsLocation.setAltitude(3.3);
    gpsLocation.setGpsFix(types::QtGpsFixEnum::MODE3D);
    gpsLocation.setHeading(4.3);
    gpsLocation.setQuality(5.3);
    gpsLocation.setElevation(6.3);
    gpsLocation.setBearing(7.3);
    gpsLocation.setGpsTime(83);
    gpsLocation.setDeviceTime(93);
    gpsLocation.setTime(103);

    QList<joynr::types::QtGpsPosition> gpsPositions;
    gpsPositions.push_back(gpsPosition);
    gpsPositions.push_back(gpsPositionExt);
    gpsPositions.push_back(gpsLocation);

    QVariantList variantList = Util::convertListToVariantList(gpsPositions);
    QVariant variant(variantList);
    QString serializedGpsLocations = JsonSerializer::serialize(variant);
    LOG_DEBUG(logger, serializedGpsLocations);

//    EXPECT_EQ(gpsLocation, *deserializedGpsLocation);
}
