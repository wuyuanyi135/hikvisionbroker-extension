//
// Created by wuyua on 11/27/2020.
//

#ifndef HIKVISIONBROKER_EXTENSION_CAMERA_H
#define HIKVISIONBROKER_EXTENSION_CAMERA_H

#include <CameraParams.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <pybind11/pybind11.h>
#include <queue>
#include <string>
#include <thread>
#include <rxcpp/rx.hpp>
#include <vector>
namespace py = pybind11;
namespace rx = rxcpp;

struct raw_frame {
    std::vector<u_char> frame;
    MV_FRAME_OUT_INFO_EX frame_info;
    void copy_frame(u_char* data, MV_FRAME_OUT_INFO_EX* frame_info);
};

class camera {
 public:
    explicit camera(int id);
    virtual ~camera();

    py::bytes read_raw_frame();
    py::bytes read_preview_frame();
    py::bytes read_original_frame();
    void register_py_callback(py::function cb);

    void set_frame_rate(float frame_rate);
    void set_exposure_time(float exposure_time);
    void set_gain(float gain);
    ///
    /// \param line_selector
    /// 0:Line0
    /// 1:Line1
    /// 2:Line2
    /// 3:Line3
    /// 4:Line4
    /// \param line_inverter
    /// \param strobe_enable
    /// \param strobe_line_duration
    /// \param strobe_line_delay
    void set_io_configuration(
        int line_selector = 2,
        bool line_inverter = true,
        int strobe_enable = 1,
        int strobe_line_duration = 20,
        int strobe_line_delay = 0);

    /// Dedicated line duration setter in case the camera is driving the LED directly.
    void set_line_duration(int duration);

 public:
    void start_acquisition();
    void stop_acquisition();

 public:
    void set_jpeg_quality(int quality);
    void set_preview_quality(int quality);
    int original_jpeg_quality = 100;
    int preview_jpeg_quality = 50;

 public:
    bool acquisition_started = false;
    std::mutex raw_frame_mutex;
    raw_frame latest_frame;
    rx::subjects::subject<raw_frame> process_pipeline;

 protected:
    std::mutex preview_mutex;
    std::mutex original_mutex;

    std::vector<u_char> preview_jpeg, original_jpeg;
    py::function callback = py::none();

 protected:
    void *handle = nullptr;

 private:
    static void check_error(int ret);
    static void check_error(int ret, const std::string &fail_message);

 private:

    void compose_process_pipeline();
    void save_jpeg(std::mutex &mutex, raw_frame &frame, std::vector<u_char> &jpeg_buffer, int quality, const char *failure_message);
};

#endif//HIKVISIONBROKER_EXTENSION_CAMERA_H
