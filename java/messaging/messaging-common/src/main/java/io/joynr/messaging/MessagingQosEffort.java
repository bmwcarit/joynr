package io.joynr.messaging;

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

/**
 * Used in {@link MessagingQos} to describe how much effort should be expent on ensuring the delivery of the message.
 */
public enum MessagingQosEffort {

    /**
     * Normal effort means that the messaging system will guarantee delivery of the message, including temporarily
     * persisting the message as necessary if the receiver is not currently online. Also known as 'at least once'.
     */
    NORMAL,

    /**
     * Best effort means that the messaging system will ensure the message is sent out, but will not guarantee that
     * it is received, also meaning the message is not persisted awaiting delivery if the receiver is not currently
     * online. Also known as 'at most once' or 'fire and forget'.
     */
    BEST_EFFORT;
}
