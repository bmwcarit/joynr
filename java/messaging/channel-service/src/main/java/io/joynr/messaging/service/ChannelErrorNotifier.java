package io.joynr.messaging.service;

/*
 * #%L
 * joynr::java::messaging::channel-service
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
 * Notifier for unrecoverable channel errors. Any notifications sent to this
 * interface are meant to be handed to a server administrator as no built-in
 * recovery strategy is possible.
 * 
 * @author christina.strobel
 * 
 */
public interface ChannelErrorNotifier {

    /**
     * Alerts that a bounce proxy is unreachable for a participant.
     * 
     * @param ccid
     *            the channel that the participant tries to use for messaging
     * @param bpId
     *            the identifier of the bounce proxy
     * @param senderAddress
     *            the address of the participant trying to communication on a
     *            channel
     * @param message
     *            additional message describing the error in more detail
     */
    void alertBounceProxyUnreachable(String ccid, String bpId, String senderAddress, String message);

}
