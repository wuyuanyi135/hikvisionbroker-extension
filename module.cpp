#include "camera.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;

PYBIND11_MODULE(hikvisionbroker_extension, m) {
    m.doc() = "HIKVISION camera python extension";// optional module docstring

    py::class_<camera>(m, "Camera")
        .def(py::init<int>())
        .def("register_callback", &camera::register_py_callback)
        .def("set_frame_rate", &camera::set_frame_rate)
        .def("set_exposure_time", &camera::set_exposure_time)
        .def("set_gain", &camera::set_gain)
        .def("set_io_configuration", &camera::set_io_configuration,
             py::arg("line_selector") = 2,
             py::arg("line_inverter") = true,
             py::arg("strobe_enable") = 1,
             py::arg("strobe_line_duration") = 20,
             py::arg("strobe_line_delay") = 0)
        .def("set_line_duration", &camera::set_line_duration)
        .def("start_acquisition", &camera::start_acquisition)
        .def("stop_acquisition", &camera::stop_acquisition)
        .def("set_jpeg_quality", &camera::set_jpeg_quality)
        .def("set_preview_quality", &camera::set_preview_quality)
        .def("read_raw_frame", &camera::read_raw_frame);
}