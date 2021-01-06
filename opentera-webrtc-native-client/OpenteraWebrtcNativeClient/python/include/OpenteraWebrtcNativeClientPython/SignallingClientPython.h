#ifndef OPENTERA_WEBRTC_NATIVE_CLIENT_PYTHON_SIGNALLING_CLIENT_PYTHON_H
#define OPENTERA_WEBRTC_NATIVE_CLIENT_PYTHON_SIGNALLING_CLIENT_PYTHON_H

#include <pybind11/pybind11.h>

namespace opentera
{
    PYBIND11_EXPORT void initSignallingClientPython(pybind11::module& m);
}

#endif
