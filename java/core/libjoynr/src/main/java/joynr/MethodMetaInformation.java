package joynr;

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

import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

import javax.annotation.CheckForNull;

import com.fasterxml.jackson.databind.JsonMappingException;

/**
 * Value class representing a java method, that will later be called using reflection. Offers methods to access the
 * Method object and the joynr annotations on the parameters.
 */
public class MethodMetaInformation {
    private final Method method;
    private final List<JoynrRpcParam> joynrAnnotations;
    private final JoynrRpcReturn returnAnnotation;
    private JoynrRpcCallback callbackAnnotation;
    private int callbackIndex = -1;

    public MethodMetaInformation(Method method) throws JsonMappingException {
        this.method = method;
        if (method.getParameterTypes().length > 0) {
            joynrAnnotations = new ArrayList<JoynrRpcParam>(method.getParameterTypes().length);
            // create a list containing a list of annotations for each parameter
            List<List<Annotation>> parameterAnnotations = ReflectionUtils.findAndMergeParameterAnnotations(method);
            // for (List<Annotation> parameterAnnotation : parameterAnnotations) {
            for (int i = 0; i < parameterAnnotations.size(); i++) {
                List<Annotation> parameterAnnotation = parameterAnnotations.get(i);

                if (findCallbackAnnotation(parameterAnnotation)) {
                    callbackIndex = i;
                } else {
                    try {
                        joynrAnnotations.add(findJoynrRpcParam(parameterAnnotation));
                    } catch (JsonMappingException e) {
                        // TODO remove
                    }
                }

            }

        } else {
            joynrAnnotations = null;
        }

        returnAnnotation = ReflectionUtils.findReturnAnnotation(method);
    }

    private boolean findCallbackAnnotation(List<Annotation> parameterAnnotation) {
        for (Annotation annotation : parameterAnnotation) {
            if (annotation instanceof JoynrRpcCallback) {
                if (callbackAnnotation == null) {
                    callbackAnnotation = (JoynrRpcCallback) annotation;
                    return true;
                }
            }
        }
        return false;
    }

    private JoynrRpcParam findJoynrRpcParam(List<Annotation> parameterAnnotation) throws JsonMappingException {
        for (Annotation annotation : parameterAnnotation) {
            if (annotation instanceof JoynrRpcParam) {
                return ((JoynrRpcParam) annotation);
            }
        }
        throw new JsonMappingException("all parameters of methods that are invoked using joynr RPC need to be annotated");
    }

    public Method getMethod() {
        return method;
    }

    public String getMethodName() {
        return method.getName();
    }

    public JoynrRpcParam getJoynAnnotation(int paramIndex) {
        return joynrAnnotations.get(paramIndex);
    }

    public List<JoynrRpcParam> getJoynAnnotations() {
        return joynrAnnotations;
    }

    public Class<?>[] getClasses() {
        return method.getParameterTypes();
    }

    public boolean isMethodWithoutParameters() {
        return method.getParameterTypes().length == 0;
        //        if ((joynrAnnotations == null) || (joynrAnnotations.size() == 0)) {
        //            return true;
        //        }
        //        return false;
    }

    @CheckForNull
    public JoynrRpcReturn getReturnAnnotation() {
        return returnAnnotation;
    }

    public JoynrRpcCallback getCallbackAnnotation() {
        return callbackAnnotation;
    }

    public int getCallbackIndex() {
        return callbackIndex;
    }
}
