package io.joynr.proxy;

import java.util.Optional;

/*-
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
 * Used for stateless async callback methods in order to provide them with the context in which this reply is
 * being called, e.g. the method ID of the request for which the call is being made.
 *
 * TODO could be extended to include the custom headers from the Reply as well.
 */
public class ReplyContext {

    private final String messageId;

    public ReplyContext(Optional<String> messageId) {
        this.messageId = messageId.isPresent() ? messageId.get() : null;
    }

    public String getMessageId() {
        return messageId;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if (o == null || getClass() != o.getClass())
            return false;

        ReplyContext that = (ReplyContext) o;

        return messageId != null ? messageId.equals(that.messageId) : that.messageId == null;
    }

    @Override
    public int hashCode() {
        return messageId != null ? messageId.hashCode() : 0;
    }

    @Override
    public String toString() {
        return "ReplyContext{" + "messageId='" + messageId + '\'' + '}';
    }
}
