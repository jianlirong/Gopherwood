/********************************************************************
 * 2016 -
 * open source under Apache License Version 2.0
 ********************************************************************/
/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "platform.h"

#include "Exception.h"
#include "ExceptionInternal.h"
#include "Thread.h"

#include <cstring>
#include <cassert>
#include <sstream>

THREAD_LOCAL char ErrorMessage[ERROR_MESSAGE_BUFFER_SIZE] = "Success";

namespace Gopherwood {

function<bool(void)> ChecnOperationCanceledCallback;

namespace Internal {

void SetErrorMessage(const char *msg) {
    assert(NULL != msg);
    strncpy(ErrorMessage, msg, sizeof(ErrorMessage) - 1);
    ErrorMessage[sizeof(ErrorMessage) - 1] = 0;
}

void SetLastException(Gopherwood::exception_ptr e) {
    std::string buffer;
    const char *p;
    p = Gopherwood::Internal::GetExceptionMessage(e, buffer);
    SetErrorMessage(p);
}

bool CheckOperationCanceled() {
    if (ChecnOperationCanceledCallback && ChecnOperationCanceledCallback()) {
        THROW(GopherwoodCanceled, "Operation has been canceled by the user.");
    }

    return false;
}

const char * GetSystemErrorInfo(int eno) {
    static THREAD_LOCAL char message[64];
    char buffer[64], *pbuffer;
    pbuffer = buffer;
#ifdef STRERROR_R_RETURN_INT
    strerror_r(eno, buffer, sizeof(buffer));
#else
    pbuffer = strerror_r(eno, buffer, sizeof(buffer));
#endif
    snprintf(message, sizeof(message), "(errno: %d) %s", eno, pbuffer);
    return message;
}

static void GetExceptionDetailInternal(const Gopherwood::GopherwoodException & e,
                                       std::stringstream & ss, bool topLevel);

static void GetExceptionDetailInternal(const std::exception & e,
                                       std::stringstream & ss, bool topLevel) {
    try {
        if (!topLevel) {
            ss << "Caused by\n";
        }

        ss << e.what();
    } catch (const std::bad_alloc & e) {
        return;
    }

    try {
        Gopherwood::rethrow_if_nested(e);
    } catch (const Gopherwood::GopherwoodException & nested) {
        GetExceptionDetailInternal(nested, ss, false);
    } catch (const std::exception & nested) {
        GetExceptionDetailInternal(nested, ss, false);
    }
}

static void GetExceptionDetailInternal(const Gopherwood::GopherwoodException & e,
                                       std::stringstream & ss, bool topLevel) {
    try {
        if (!topLevel) {
            ss << "Caused by\n";
        }

        ss << e.msg();
    } catch (const std::bad_alloc & e) {
        return;
    }

    try {
        Gopherwood::rethrow_if_nested(e);
    } catch (const Gopherwood::GopherwoodException & nested) {
        GetExceptionDetailInternal(nested, ss, false);
    } catch (const std::exception & nested) {
        GetExceptionDetailInternal(nested, ss, false);
    }
}

const char* GetExceptionDetail(const Gopherwood::GopherwoodException& e,
                               std::string& buffer) {
    try {
        std::stringstream ss;
        ss.imbue(std::locale::classic());
        GetExceptionDetailInternal(e, ss, true);
        buffer = ss.str();
    } catch (const std::bad_alloc& e) {
        return "Out of memory";
    }

    return buffer.c_str();
}

const char* GetExceptionDetail(const exception_ptr e, std::string& buffer) {
    std::stringstream ss;
    ss.imbue(std::locale::classic());

    try {
        Gopherwood::rethrow_exception(e);
    } catch (const Gopherwood::GopherwoodException& nested) {
        GetExceptionDetailInternal(nested, ss, true);
    } catch (const std::exception& nested) {
        GetExceptionDetailInternal(nested, ss, true);
    }

    try {
        buffer = ss.str();
    } catch (const std::bad_alloc& e) {
        return "Out of memory";
    }

    return buffer.c_str();
}

static void GetExceptionMessage(const std::exception & e,
                                std::stringstream & ss, int recursive) {
    try {
        for (int i = 0; i < recursive; ++i) {
            ss << '\t';
        }

        if (recursive > 0) {
            ss << "Caused by: ";
        }

        ss << e.what();
    } catch (const std::bad_alloc & e) {
        return;
    }

    try {
        Gopherwood::rethrow_if_nested(e);
    } catch (const std::exception & nested) {
        GetExceptionMessage(nested, ss, recursive + 1);
    }
}

const char * GetExceptionMessage(const exception_ptr e, std::string & buffer) {
    std::stringstream ss;
    ss.imbue(std::locale::classic());

    try {
        Gopherwood::rethrow_exception(e);
    } catch (const std::bad_alloc & e) {
        return "Out of memory";
    } catch (const std::exception & e) {
        GetExceptionMessage(e, ss, 0);
    }

    try {
        buffer = ss.str();
    } catch (const std::bad_alloc & e) {
        return "Out of memory";
    }

    return buffer.c_str();
}

}
}

