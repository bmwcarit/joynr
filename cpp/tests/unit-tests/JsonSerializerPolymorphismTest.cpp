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
#include "joynr/types/GpsLocation.h"
#include "joynr/types/Location.h"
#include "joynr/types/GpsFixEnum.h"
#include "joynr/types/Trip.h"
#include "joynr/JsonSerializer.h"
#include "joynr/joynrlogging.h"
#include "joynr/DeclareMetatypeUtil.h"
#include "joynr/Util.h"
#include <QList>

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

TEST_F(JsonSerializerPolymorphismTest, deserializeGpsLocationFromLocation) {
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");
    qRegisterMetaType<joynr::types::Location>("joynr::types::Location");
    qRegisterMetaType<joynr__types__Location>("joynr__types__Location");

    joynr::types::GpsLocation expectedGpsLocation;
    expectedGpsLocation.setGpsFix(types::GpsFixEnum::MODE3D);
    expectedGpsLocation.setLongitude(1.1);
    expectedGpsLocation.setLatitude(2.2);
    expectedGpsLocation.setAltitude(3.3);
    expectedGpsLocation.setTime(17);

//    joynr::types::Location location;
//    location.setGpsFix(types::GpsFixEnum::MODE3D);
//    location.setLongitude(1.1);
//    location.setLatitude(2.2);
//    location.setAltitude(3.3);
//    location.setTime(17);
//    location.setDescription("location.description");
//    location.setCountry("location.country");
//    location.setCity("location.city");
//    location.setStreet("location.street");
//    location.setStreetNumber("location.streetNumber");
//    location.setPostalCode("location.postalCode");
//    location.setTitle("location.title");

    QByteArray serializedLocation(
            "{ "
                "\"_typeName\" : \"joynr.types.Location\", "
                "\"altitude\" : 3.3, "
                "\"city\" : \"location.city\", "
                "\"country\" : \"location.country\", "
                "\"description\" : \"location.description\", "
                "\"gpsFix\" : \"MODE3D\", "
                "\"latitude\" : 2.2, "
                "\"longitude\" : 1.1, "
                "\"postalCode\" : \"location.postalCode\", "
                "\"street\" : \"location.street\", "
                "\"streetNumber\" : \"location.streetNumber\", "
                "\"time\" : 17, "
                "\"title\" : \"location.title\" "
            "}"
    );
    joynr::types::GpsLocation* deserializedGpsLocation = JsonSerializer::deserialize<joynr::types::GpsLocation>(serializedLocation);

    EXPECT_EQ(expectedGpsLocation, *deserializedGpsLocation);

    delete deserializedGpsLocation;
}

TEST_F(JsonSerializerPolymorphismTest, serializeGpsLocationListWithLocationInside) {
    qRegisterMetaType<joynr::types::GpsLocation>("joynr::types::GpsLocation");
    qRegisterMetaType<joynr__types__GpsLocation>("joynr__types__GpsLocation");
    qRegisterMetaType<joynr::types::Location>("joynr::types::Location");
    qRegisterMetaType<joynr__types__Location>("joynr__types__Location");

    joynr::types::GpsLocation gpsLocation;
    gpsLocation.setGpsFix(types::GpsFixEnum::MODE3D);
    gpsLocation.setLongitude(1.1);
    gpsLocation.setLatitude(1.2);
    gpsLocation.setAltitude(1.3);
    gpsLocation.setTime(14);

    joynr::types::Location location;
    location.setGpsFix(types::GpsFixEnum::MODE3D);
    location.setLongitude(2.1);
    location.setLatitude(2.2);
    location.setAltitude(2.3);
    location.setTime(24);
    location.setDescription("location.description");
    location.setCountry("location.country");
    location.setCity("location.city");
    location.setStreet("location.street");
    location.setStreetNumber("location.streetNumber");
    location.setPostalCode("location.postalCode");
    location.setTitle("location.title");

    QList<joynr::types::GpsLocation> gpsLocations;
    gpsLocations.push_back(location);
    gpsLocations.push_back(gpsLocation);

    QVariantList variantList = Util::convertListToVariantList(gpsLocations);
    QVariant variant(variantList);
    QString serializedGpsLocations = JsonSerializer::serialize(variant);
    LOG_DEBUG(logger, serializedGpsLocations);

//    EXPECT_EQ(gpsLocation, *deserializedGpsLocation);
}
