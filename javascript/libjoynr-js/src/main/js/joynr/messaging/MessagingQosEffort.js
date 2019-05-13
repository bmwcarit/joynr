/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

/**
 * The messaging QoS effort can be used to define how much effort should
 * be expended on ensuring delivery of a message. See the individual
 * members for a description of what each one means.
 */
const MessagingQosEffort = {};

/**
 * Normal effort means that the messaging system will guarantee delivery of the message, including temporarily
 * persisting the message as necessary if the receiver is not currently online. Also known as 'at least once'.
 */
MessagingQosEffort.NORMAL = {
    name: "Normal",
    value: "NORMAL"
};

/**
 * Best effort means that the messaging system will ensure the message is sent out, but will not guarantee that
 * it is received, also meaning the message is not persisted awaiting delivery if the receiver is not currently
 * online. Also known as 'at most once' or 'fire and forget'.
 */
MessagingQosEffort.BEST_EFFORT = {
    name: "Best effort",
    value: "BEST_EFFORT"
};

/**
 * Helper method to allow checking a value to see if it is a valid member of this enum.
 */
MessagingQosEffort.isValid = function(value) {
    return value === this.NORMAL || value === this.BEST_EFFORT;
};

module.exports = MessagingQosEffort;
