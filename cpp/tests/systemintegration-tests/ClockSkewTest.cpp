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
#include "joynr/PrivateCopyAssign.h"
#include "joynr/MessagingSettings.h"
#include "joynr/BrokerUrl.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/Logger.h"
#include "joynr/Settings.h"

#include <chrono>
#include <gtest/gtest.h>
#include <curl/curl.h>
#include <regex>

using namespace joynr;

using namespace ::testing;

// A date string extracted from a HTTP header
static char datestr[80];

class ClockSkewTest : public Test {
public:
    Settings settings;
    MessagingSettings messagingSettings;

    ClockSkewTest() :
        settings("test-resources/HttpSystemIntegrationTest1.settings"),
        messagingSettings(settings)
    {
    }

private:
    DISALLOW_COPY_AND_ASSIGN(ClockSkewTest);
};

// Function called by libcurl for each header line
size_t getHTTPHeaderDate(void *ptr, size_t size, size_t nmemb, void *userdata) {

    // Initialize the static character array
    datestr[0] = '\0';

    // Extract the date string from the date HTTP header
    if (strncmp((char *) ptr, "Date: ", size) == 0) {
       strncpy(datestr, ((char *) ptr) + 6, sizeof(datestr) -1);
       datestr[sizeof(datestr) - 1] = '\0';
       return 0;
    }

    // Signal to curl that the header line was handled correctly
    return size * nmemb;
}

// A test to see if the local clock is out of sync with the bounce proxy.
// This was added because the system integration tests fail when clocks are out of sync.
// The test failures are caused by absolute time to live (TTL) values that
// expire messages too soon when clocks aren't in sync.

// TODO reenable test
TEST_F(ClockSkewTest, DISABLED_checkClockSkew) {
    Logger logger("ClockSkewTest");

    // Get the location of the bounce proxy
    Url bounceurl = messagingSettings.getBrokerUrl().getTimeCheckUrl();
    ASSERT_TRUE(bounceurl.isValid());
    std::string urlString = bounceurl.toString();
    const char *url  = urlString.c_str();

    // Use libcurl to get the HTTP date from the bounce proxy server
    CURL *curl = curl_easy_init();
    if (!curl) {
        JOYNR_LOG_ERROR(logger, "unknown error during curl_easy_init");
        FAIL();
    }

    // The line below contains a macro that is marked invalid by the QTCreator parser
    // but compiles without problem
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, getHTTPHeaderDate);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);

    // This call will fail but only the date header is interesting
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    // Was a date obtained?
    ASSERT_FALSE(datestr[0] == '\0') << "Could not read date from bounce proxy";

    // Parse the returned date using curl
    time_t epochsecs = curl_getdate(datestr, nullptr);

    ASSERT_FALSE(epochsecs < -1) << "Could not parse date from bounce proxy.";

    // Compare the time with the local time
    std::uint64_t now        = DispatcherUtils::nowInMilliseconds();
    std::uint64_t remoteTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::from_time_t(epochsecs).time_since_epoch()).count();

    auto minMaxTime = std::minmax(now, remoteTime);
    std::uint64_t diff = minMaxTime.second - minMaxTime.first;

    JOYNR_LOG_INFO(logger, "Time difference is {}  msecs",now);
    EXPECT_TRUE(diff < 2000) << "Time difference between local and remote is over 2 seconds";

}
