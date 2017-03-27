package io.joynr.messaging.filter;

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

import java.io.IOException;

import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

public class CorsFilter implements Filter {
    @Override
    public void doFilter(ServletRequest request, ServletResponse response, FilterChain chain) throws IOException,
                                                                                             ServletException {
        HttpServletRequest req = (HttpServletRequest) request;
        HttpServletResponse res = (HttpServletResponse) response;

        if (req.getHeader("Origin") != null
                && ("POST".equals(req.getMethod()) || "OPTIONS".equals(req.getMethod()) || "DELETE".equals(req.getMethod()))) {
            res.addHeader("Access-Control-Allow-Origin", "*");
        }

        res.addHeader("Access-Control-Max-Age", "-1");
        res.addHeader("Access-Control-Expose-Headers",
                      "X-Cache-Date, X-Cache-Index, X-Atmosphere-tracking-id, Location");

        if ("OPTIONS".equals(req.getMethod())) {
            res.addHeader("Access-Control-Allow-Methods", "OPTIONS, GET, POST, DELETE");
            res.addHeader("Access-Control-Allow-Headers",
                          "Content-Type, X-Cache-Index,  X-Atmosphere-tracking-id, BMW-Vin");
            //        "X-Atmosphere-Framework, X-Cache-Date, X-Cache-Index, BMW-Vin, X-Atmosphere-tracking-id, X-Atmosphere-Transport");
            //                      ", Accept-Charset, Accept-Encoding, Host, Proxy-Connection, Referer, User-Agent, Content-Type");
        }

        chain.doFilter(req, res);
    }

    @Override
    public void destroy() {
    }

    @Override
    public void init(FilterConfig filterConfig) throws ServletException {
    }
}