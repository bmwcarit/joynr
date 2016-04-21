/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

#include <vector>
#include <numeric>
#include <string>

#include <boost/type_index.hpp>

#include "../common/PerformanceTest.h"
#include "joynr/Request.h"
#include "joynr/TypeUtil.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"
#include "joynr/JsonSerializer.h"

#include "joynr/tests/performance/Types/ComplexStruct.h"
#include "joynr/tests/performance/Types/ComplexStructSerializer.h"

template <typename Generator>
class SerializerPerformanceTest : public PerformanceTest<>
{
public:
    SerializerPerformanceTest(std::size_t length)
            : length(length), request(Generator::generateRequest(length)), messageFactory(), qos()
    {
    }

    void runSerializationBenchmark() const
    {
        auto fun = [this]() { this->createMessage(); };
        runAndPrintAverage(getTestName("serialization"), fun);
    }

    void runFullMessageSerializationBenchmark() const
    {
        auto fun = [this]() {
            joynr::JoynrMessage message = this->createMessage();
            return joynr::JsonSerializer::serialize(message);
        };
        runAndPrintAverage(getTestName("full message serialization"), fun);
    }

    void runDeSerializationBenchmark() const
    {
        joynr::JoynrMessage message = createMessage();
        auto fun = [&message]() {
            return joynr::JsonSerializer::deserialize<joynr::Request>(message.getPayload());
        };

        runAndPrintAverage(getTestName("deserialization"), fun);
    }

    void runFullMessageDeSerializationBenchmark() const
    {
        // create a message, serialize it and then benchmark the deserialization
        joynr::JoynrMessage message = createMessage();
        std::string serializedMessage = joynr::JsonSerializer::serialize(message);

        auto fun = [&serializedMessage]() {
            joynr::JoynrMessage msg =
                    joynr::JsonSerializer::deserialize<joynr::JoynrMessage>(serializedMessage);
            joynr::Request req =
                    joynr::JsonSerializer::deserialize<joynr::Request>(msg.getPayload());
            return req;
        };

        runAndPrintAverage(getTestName("full message deserialization"), fun);
    }

private:
    joynr::JoynrMessage createMessage() const
    {
        return messageFactory.createRequest(
                senderParticipantId, receiverParticipantId, qos, request);
    }

    std::string getTestName(const std::string& testType) const
    {
        return testType + " " + boost::typeindex::type_id<Generator>().pretty_name() + " length=" +
               std::to_string(length);
    }

    std::size_t length;
    joynr::Request request;
    joynr::JoynrMessageFactory messageFactory;
    joynr::MessagingQos qos;

    const std::string senderParticipantId = "sender";
    const std::string receiverParticipantId = "receiver";
};

namespace generator
{

namespace helper
{
std::vector<std::int8_t> getFilledVector(std::size_t length)
{
    std::vector<std::int8_t> data(length);
    // fill data with sequentially increasing numbers
    std::iota(data.begin(), data.end(), 0);
    return data;
}

std::string getFilledString(std::size_t length)
{
    return std::string(length, '#');
}
} // namespace helper

struct ByteArray
{
    static joynr::Request generateRequest(std::size_t length)
    {
        joynr::Request request;
        request.addParam(
                joynr::TypeUtil::toVariant<std::int8_t>(helper::getFilledVector(length)), "Byte[]");
        request.setMethodName("echoByteArray");
        return request;
    }
};

struct String
{
    static joynr::Request generateRequest(std::size_t length)
    {
        joynr::Request request;
        request.addParam(
                joynr::Variant::make<std::string>(helper::getFilledString(length)), "String");
        request.setMethodName("echoString");
        return request;
    }
};

struct ComplexStruct
{
    static joynr::Request generateRequest(std::size_t length)
    {
        joynr::Request request;
        using joynr::tests::performance::Types::ComplexStruct;
        ComplexStruct complexStruct(
                32, 64, helper::getFilledVector(length), helper::getFilledString(length));
        request.addParam(joynr::Variant::make<ComplexStruct>(complexStruct),
                         "joynr.tests.performance.Types.ComplexStruct");
        request.setMethodName("echoComplexStruct");
        return request;
    }
};

} // namespace generator
