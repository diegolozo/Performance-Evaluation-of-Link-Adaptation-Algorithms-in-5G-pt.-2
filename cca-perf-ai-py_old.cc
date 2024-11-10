#include "ns3/lte-rlc-um.h"

#include <ns3/ai-module.h>

#include <iostream>
#include <pybind11/pybind11.h>
#include <boost/python.hpp>

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector);
PYBIND11_MAKE_OPAQUE(ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector);

PYBIND11_MODULE(cca_perf_py, m)
{
    py::class_<ns3::LteRlcUm::dataToSend>(m, "bufferData")
        .def(py::init<>())
        .def_readwrite("delay", &ns3::LteRlcUm::dataToSend::bufferDelay)
        .def_readwrite("fill_perc", &ns3::LteRlcUm::dataToSend::bufferFillPerc)
        .def_readwrite("just_called", &ns3::LteRlcUm::dataToSend::justCalled)
        .def_readwrite("entering", &ns3::LteRlcUm::dataToSend::entering)
        .def_readwrite("exiting", &ns3::LteRlcUm::dataToSend::exiting)
        .def_readwrite("simulation_time", &ns3::LteRlcUm::dataToSend::simulationTime)
        .def_readwrite("emptying_time", &ns3::LteRlcUm::dataToSend::emptyingTime)
        .def_readwrite("is_buffer_full", &ns3::LteRlcUm::dataToSend::isBufferFull)
        .def_readwrite("is_fragmented", &ns3::LteRlcUm::dataToSend::isFragmented);;


    py::class_<ns3::LteRlcUm::dataToRecv>(m, "bufferDecision").def(py::init<>()).def_readwrite("drop_packet", &ns3::LteRlcUm::dataToRecv::dropPacket);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector>(m, "bufferDataVector")
        .def(
            "resize",
            static_cast<void (ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector::*)(
                ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector::size_type)>(
                &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector::size)
        .def(
            "__getitem__",
            [](ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Cpp2PyMsgVector& vec,
               uint32_t i) -> ns3::LteRlcUm::dataToSend& {
                if (i >= vec.size())
                {
                    std::cerr << "Invalid index " << i << " for buffer data vector, whose size is "
                              << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            },
            py::return_value_policy::reference);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector>(m, "bufferDecisionVector")
        .def(
            "resize",
            static_cast<void (ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector::*)(
                ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector::size_type)>(
                &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector::resize))
        .def("__len__", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector::size)
        .def(
            "__getitem__",
            [](ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::Py2CppMsgVector& vec,
               uint32_t i) -> ns3::LteRlcUm::dataToRecv& {
                if (i >= vec.size())
                {
                    std::cerr << "Invalid index " << i << " for buffer decision vector, whose size is "
                              << vec.size() << std::endl;
                    exit(1);
                }
                return vec.at(i);
            },
            py::return_value_policy::reference);

    py::class_<ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool,
                      bool,
                      bool,
                      uint32_t,
                      const char*,
                      const char*,
                      const char*,
                      const char*>())
        .def("PyRecvBegin", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::PyRecvBegin)
        .def("PyRecvEnd", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::PyRecvEnd)
        .def("PySendBegin", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::PySendBegin)
        .def("PySendEnd", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::PySendEnd)
        .def("PyGetFinished", &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::PyGetFinished)
        .def("GetCpp2PyVector",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::GetCpp2PyVector,
             py::return_value_policy::reference)
        .def("GetPy2CppVector",
             &ns3::Ns3AiMsgInterfaceImpl<ns3::LteRlcUm::dataToSend, ns3::LteRlcUm::dataToRecv>::GetPy2CppVector,
             py::return_value_policy::reference);
}
