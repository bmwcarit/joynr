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
package io.joynr.integration.util;

import java.util.LinkedList;
import java.util.List;

import com.gargoylesoftware.htmlunit.WebClient;
import com.gargoylesoftware.htmlunit.html.DomElement;
import com.gargoylesoftware.htmlunit.html.HtmlPage;

public class ChannelServiceTestUtils {

    private static final String TEXT_NO_DATA_AVAILABLE = "No data available in table";

    /**
     * Returns a list of channel IDs that are displayed on the channels.html page for the bounce proxy.
     * 
     * @param webClient
     * @param url
     * @return
     * @throws Exception
     */
    public static List<String> getChannelIdsOnChannelsHtml(WebClient webClient, String url) throws Exception {

        HtmlPage page = webClient.getPage(url);
        webClient.waitForBackgroundJavaScript(2000);

        DomElement channelsTable = page.getElementById("channels");

        List<String> channelIds = new LinkedList<String>();
        for (DomElement channelsTableRows : channelsTable.getChildElements()) {
            if (channelsTableRows.getTagName().equals("tbody")) {

                for (DomElement channelRows : channelsTableRows.getChildElements()) {

                    String channelId = channelRows.getChildNodes().get(0).getTextContent();
                    if (isProperChannelId(channelId)) {
                        channelIds.add(channelId);
                    }
                }
            }
        }
        return channelIds;
    }

    private static boolean isProperChannelId(String channelId) {
        return !channelId.equals(TEXT_NO_DATA_AVAILABLE);
    }
}
