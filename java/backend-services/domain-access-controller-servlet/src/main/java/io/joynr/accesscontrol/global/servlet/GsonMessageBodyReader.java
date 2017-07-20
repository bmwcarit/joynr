package io.joynr.accesscontrol.global.servlet;

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

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.inject.Singleton;

import javax.ws.rs.Consumes;
import javax.ws.rs.Produces;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.ext.MessageBodyReader;
import javax.ws.rs.ext.MessageBodyWriter;
import javax.ws.rs.ext.Provider;
import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.OutputStreamWriter;
import java.io.InputStreamReader;
import java.lang.annotation.Annotation;
import java.lang.reflect.Type;

@Provider
@Singleton
@Consumes(MediaType.APPLICATION_JSON)
@Produces(MediaType.APPLICATION_JSON)
public class GsonMessageBodyReader implements MessageBodyWriter<Object>, MessageBodyReader<Object> {

    private static final String UTF_8 = "UTF-8";

    private static Gson gson;

    private Gson getGsonInstance() {
        if (gson == null) {
            final GsonBuilder gsonBuilder = new GsonBuilder();
            gson = gsonBuilder.create();
        }
        return gson;
    }

    @Override
    public boolean isReadable(Class<?> type,
                              Type genericType,
                              java.lang.annotation.Annotation[] annotations,
                              MediaType mediaType) {
        return true;
    }

    @Override
    public Object readFrom(Class<Object> type,
                           Type genericType,
                           Annotation[] annotations,
                           MediaType mediaType,
                           MultivaluedMap<String, String> httpHeaders,
                           InputStream entityStream) throws IOException {
        InputStreamReader streamReader = null;
        try {
            streamReader = new InputStreamReader(entityStream, UTF_8);
            Type jsonType;
            if (type.equals(genericType)) {
                jsonType = type;
            } else {
                jsonType = genericType;
            }
            return getGsonInstance().fromJson(streamReader, jsonType);
        } finally {
            if (streamReader != null) {
                streamReader.close();
            }
        }
    }

    @Override
    public boolean isWriteable(Class<?> type, Type genericType, Annotation[] annotations, MediaType mediaType) {
        return true;
    }

    @Override
    public long getSize(Object object, Class<?> type, Type genericType, Annotation[] annotations, MediaType mediaType) {
        return -1;
    }

    @Override
    public void writeTo(Object object,
                        Class<?> type,
                        Type genericType,
                        Annotation[] annotations,
                        MediaType mediaType,
                        MultivaluedMap<String, Object> httpHeaders,
                        OutputStream entityStream) throws IOException, WebApplicationException {
        OutputStreamWriter writer = null;
        try {
            writer = new OutputStreamWriter(entityStream, UTF_8);
            Type jsonType;
            if (type.equals(genericType)) {
                jsonType = type;
            } else {
                jsonType = genericType;
            }
            getGsonInstance().toJson(object, jsonType, writer);
        } finally {
            if (writer != null) {
                writer.close();
            }
        }
    }
}
