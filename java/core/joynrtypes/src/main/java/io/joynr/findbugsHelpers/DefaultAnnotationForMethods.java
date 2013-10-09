package io.joynr.findbugsHelpers;

/*
 * #%L
 * joynr::java::messaging::messagingcommon
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

/*
 * FindBugs - Find Bugs in Java programs
 * Copyright (C) 2005, University of Maryland
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

import java.lang.annotation.Annotation;
import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import edu.umd.cs.findbugs.annotations.Confidence;
import edu.umd.cs.findbugs.annotations.Priority;

/**
 * Indicates that all members of the class or package should be annotated with
 * the default value of the supplied annotation class.
 * 
 * This would be used for behavior annotations such as @NonNull, @CheckForNull,
 * or @CheckReturnValue.
 * 
 * In particular, you can use @DefaultAnnotation(NonNull.class) on a class or
 * package, and then use @Nullable only on those parameters, methods or fields
 * that you want to allow to be null.
 * 
 * @author William Pugh
 */

@SuppressWarnings("deprecation")
@Documented
@Target({ ElementType.TYPE, ElementType.PACKAGE })
@Retention(RetentionPolicy.CLASS)
public @interface DefaultAnnotationForMethods {
    Class<? extends Annotation>[] value();

    @Deprecated
    Priority priority() default Priority.MEDIUM;

    Confidence confidence() default Confidence.MEDIUM;
}
