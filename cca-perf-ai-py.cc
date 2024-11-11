#include "ns3/nr-amc.h"

#include <ns3/ai-module.h>

#include <iostream>
#include <pybind11/pybind11.h>
#include <boost/python.hpp>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector);
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector);

PYBIND11_MODULE(cca_perf_py, m)
{
    py::class_<ns3::NrAmc::dataToSend>(m, "bufferData")
        .def(py::init<>())
        .def_readwrite("sinr_eff", &ns3::NrAmc::dataToSend::sinrEffective)
        .def_readwrite("simulation_time", &ns3::NrAmc::dataToSend::simulationTime);;


    py::class_<ns3::NrAmc::dataToRecv>(m, "bufferDecision")
        .def(py::init<>())
        .def_readwrite("bler_target", &ns3::NrAmc::dataToRecv::blerTarget);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector>(m, "bufferDataVector")
        .def(
            "resize",
            static_cast<void (ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector::*)(
                ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector::size_type)>(
                &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector::size)
        .def(
            "__getitem__",
            [](ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Cpp2PyMsgVector& vec,
               uint32_t i) -> ns3::NrAmc::dataToSend& {
                if (i >= vec.size())
                {
                    std::cerr << "Invalid index " << i << " for buffer data vector, whose size is "
                              << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            },
            py::return_value_policy::reference);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector>(m, "bufferDecisionVector")
        .def(
            "resize",
            static_cast<void (ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector::*)(
                ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector::size_type)>(
                &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector::size)
        .def(
            "__getitem__",
            [](ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::Py2CppMsgVector& vec,
               uint32_t i) -> ns3::NrAmc::dataToRecv& {
                if (i >= vec.size())
                {
                    std::cerr << "Invalid index " << i << " for buffer decision vector, whose size is "
                              << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            },
            py::return_value_policy::reference);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::PyRecvBegin)
        .def("PyRecvEnd", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::PyRecvEnd)
        .def("PySendBegin", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::PySendBegin)
        .def("PySendEnd", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::PySendEnd)
        .def("PyGetFinished", &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::PyGetFinished)
        .def("GetCpp2PyVector",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::GetCpp2PyVector,
             py::return_value_policy::reference)
        .def("GetPy2CppVector",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::NrAmc::dataToSend, ns3::NrAmc::dataToRecv>::GetPy2CppVector,
             py::return_value_policy::reference);
}
