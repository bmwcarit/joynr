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
#ifndef JOYNRLOGGINGMACROS_H_
#define JOYNRLOGGINGMACROS_H_

#ifdef JOYNR_MAX_LOG_LEVEL_FATAL
#define LOG_TRACE(logger, message)
#define LOG_DEBUG(logger, message)
#define LOG_INFO(logger, message)
#define LOG_WARN(logger, message)
#define LOG_ERROR(logger, message)
#endif // JOYNR_MAX_LOG_LEVEL_FATAL

#ifdef JOYNR_MAX_LOG_LEVEL_ERROR
#define LOG_TRACE(logger, message)
#define LOG_DEBUG(logger, message)
#define LOG_INFO(logger, message)
#define LOG_WARN(logger, message)
#endif // JOYNR_MAX_LOG_LEVEL_ERROR

#ifdef JOYNR_MAX_LOG_LEVEL_WARN
#define LOG_TRACE(logger, message)
#define LOG_DEBUG(logger, message)
#define LOG_INFO(logger, message)
#endif // JOYNR_MAX_LOG_LEVEL_WARN

#ifdef JOYNR_MAX_LOG_LEVEL_INFO
#define LOG_TRACE(logger, message)
#define LOG_DEBUG(logger, message)
#endif // JOYNR_MAX_LOG_LEVEL_INFO

#ifdef JOYNR_MAX_LOG_LEVEL_DEBUG
#define LOG_TRACE(logger, message)
#endif // JOYNR_MAX_LOG_LEVEL_DEBUG

#ifndef LOG_TRACE
#define LOG_TRACE(logger, message) logger->log(joynr::joynr_logging::TRACE, message)
#endif // LOG_TRACE

#ifndef LOG_DEBUG
#define LOG_DEBUG(logger, message) logger->log(joynr::joynr_logging::DEBUG, message)
#endif // LOG_DEBUG

#ifndef LOG_INFO
#define LOG_INFO(logger, message) logger->log(joynr::joynr_logging::INFO, message)
#endif // LOG_INFO

#ifndef LOG_WARN
#define LOG_WARN(logger, message) logger->log(joynr::joynr_logging::WARN, message)
#endif // LOG_WARN

#ifndef LOG_ERROR
#define LOG_ERROR(logger, message) logger->log(joynr::joynr_logging::ERROR, message)
#endif // LOG_ERROR

#ifndef LOG_FATAL
#define LOG_FATAL(logger, message) logger->log(joynr::joynr_logging::FATAL, message)
#endif // LOG_FATAL

#endif // JOYNRLOGGINGMACROS_H_
