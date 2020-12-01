//
// Created by wuyua on 11/27/2020.
//

#include "camera.h"
#include <CameraParams.h>
#include <MvCameraControl.h>
#include <cstring>
#include <exception>
#include <iostream>
#include <jpeglib.h>
#include <stdexcept>

void __stdcall image_callback(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser) {
    if (pFrameInfo) {
        auto *camera_instance = static_cast<camera *>(pUser);
        {
            std::unique_lock<std::mutex> lock(camera_instance->raw_frame_mutex, std::try_to_lock);

            if (lock.owns_lock()) {

                // No conflicting access, can write.
                camera_instance->latest_frame.copy_frame(pData, pFrameInfo);

                lock.unlock();

                camera_instance->process_pipeline.get_subscriber().on_next(camera_instance->latest_frame);

            } else {
                std::cerr << "Reentrance detected. Performance degraded." << std::endl;
            }
        }
    }
}

camera::camera(int id) {
    compose_process_pipeline();

    MV_CC_DEVICE_INFO_LIST device_list;
    memset(&device_list, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int ret = MV_CC_EnumDevices(MV_USB_DEVICE, &device_list);
    check_error(ret, "Enum device");

    if (device_list.nDeviceNum <= id) {
        throw std::runtime_error("Camera id out of bound");
    }

    ret = MV_CC_CreateHandle(&handle, device_list.pDeviceInfo[id]);
    check_error(ret, "Create handle");

    // register image callback
    ret = MV_CC_RegisterImageCallBackEx(handle, image_callback, this);
    check_error(ret, "Image callback");

    ret = MV_CC_OpenDevice(handle);
    check_error(ret, "Open Device");

    ret = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    check_error(ret, "Trigger mode");
}

void camera::check_error(int ret, const std::string &fail_message) {
    if (MV_OK != ret) {
        throw std::runtime_error(fail_message);
    }
}
void camera::check_error(int ret) {
    check_error(ret, "");
}
void camera::set_frame_rate(float frame_rate) {
    int ret = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", frame_rate);
    check_error(ret, "Failed to set frame rate");
}
void camera::set_exposure_time(float exposure_time) {
    int ret = MV_CC_SetFloatValue(handle, "ExposureTime", exposure_time);
    check_error(ret, "Failed to set exposure time");
}
void camera::set_gain(float gain) {
    int ret = MV_CC_SetFloatValue(handle, "Gain", gain);
    check_error(ret, "Failed to set gain");
}
void camera::set_io_configuration(int line_selector, bool line_inverter, int strobe_enable, int strobe_line_duration, int strobe_line_delay) {
    int ret = MV_CC_SetEnumValue(handle, "LineSelector", line_selector);
    check_error(ret, "Failed to set line selector");

    ret = MV_CC_SetEnumValue(handle, "LineMode", 8); // strobe
    check_error(ret, "Failed to set line mode to strobe");

    ret = MV_CC_SetBoolValue(handle, "LineInverter", line_inverter);
    check_error(ret, "Failed to set line inverter");

    ret = MV_CC_SetBoolValue(handle, "StrobeEnable", bool(strobe_enable));
    check_error(ret, "Failed to set strobe enable");

    set_line_duration(strobe_line_duration);

    ret = MV_CC_SetIntValue(handle, "StrobeLineDelay", strobe_line_delay);
    check_error(ret, "Failed to set strobe delay");
}

void camera::set_line_duration(int duration) {
    int ret = MV_CC_SetIntValue(handle, "StrobeLineDuration", duration);
    check_error(ret, "Failed to set strobe duration");
}


void camera::start_acquisition() {
    int ret = MV_CC_StartGrabbing(handle);
    check_error(ret, "Failed to start acquisition");
    acquisition_started = true;
}
void camera::stop_acquisition() {
    int ret = MV_CC_StopGrabbing(handle);
    check_error(ret, "Failed to stop acquisition");
    acquisition_started = false;
}
camera::~camera() {
    try {
        stop_acquisition();
    } catch (std::exception &ex) {
    }

    try {
        MV_CC_CloseDevice(handle);
    } catch (std::exception &ex) {
    }

    try {
        MV_CC_DestroyHandle(handle);
    } catch (std::exception &ex) {
    }
}
void camera::set_jpeg_quality(int quality) {
    if (quality > 100 || quality <= 0) {
        throw std::invalid_argument("quality should be between 0 and 100");
    }
    original_jpeg_quality = quality;
}

void camera::set_preview_quality(int quality) {
    preview_jpeg_quality = quality;
}

py::bytes camera::read_raw_frame() {
    std::lock_guard<std::mutex> lock(raw_frame_mutex);
    auto s = std::string(latest_frame.frame.begin(), latest_frame.frame.end());
    return py::bytes(s);
}

py::bytes camera::read_preview_frame() {
    std::lock_guard<std::mutex> lock(preview_mutex);
    auto buf = std::string(preview_jpeg.begin(), preview_jpeg.end());
    return py::bytes(buf);
}

py::bytes camera::read_original_frame() {
    std::lock_guard<std::mutex> lock(original_mutex);
    auto buf = std::string(original_jpeg.begin(), original_jpeg.end());
    return py::bytes(buf);
}

void camera::register_py_callback(py::function cb) {
    callback = std::move(cb);
}

void camera::compose_process_pipeline() {
    // Starting by lock
    auto observable =
        process_pipeline
            .get_observable()
            .observe_on(worker)
            .tap([&](raw_frame &frame) {
                raw_frame_mutex.lock();
            })
            .publish();

    // preview line
    auto preview_line_observable =
        observable
            .map([&](raw_frame &frame) -> std::vector<u_char> & {
                save_jpeg(preview_mutex, frame, preview_jpeg, preview_jpeg_quality, "Failed to convert preview jpeg");
                return preview_jpeg;
            });
    // original line
    auto original_line_observable =
        observable
            //            .observe_on(rx::observe_on_new_thread())
            .map([&](raw_frame &frame) -> std::vector<u_char> & {
                save_jpeg(original_mutex, frame, original_jpeg, original_jpeg_quality, "Failed to convert original jpeg");
                return original_jpeg;
            });

    auto zipped = preview_line_observable.zip(original_line_observable);

    // send result to python callback (if any)
    zipped.subscribe(
        [&](auto zipped) {
            raw_frame_mutex.unlock();
            py::gil_scoped_acquire acquire;
            try {

                if (!callback.is_none()) {

                    callback(py::make_tuple(read_preview_frame(), read_original_frame()));
                }
            } catch (py::error_already_set &ex) {
                py::print("Error in python callback. The acquisition will be stopped.");
                py::print(ex.trace());
                stop_acquisition();
            }
        },
        [&](std::exception_ptr ep) {
            raw_frame_mutex.unlock();
            try {
                std::rethrow_exception(ep);
            } catch (const std::exception &ex) {
                std::cerr << "OnError: " << ex.what() << std::endl;
            }
            stop_acquisition();
        });
    observable.connect();// allow shared observable to emit.
}
void camera::save_jpeg(std::mutex &mutex, raw_frame &frame, std::vector<u_char> &jpeg_buffer, int quality, const char *failure_message) {
    std::lock_guard<std::mutex> lock(mutex);
    const MV_FRAME_OUT_INFO_EX &frame_info = frame.frame_info;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPROW row_pointer[1];
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);
    cinfo.image_width = frame_info.nWidth;
    cinfo.image_height = frame_info.nHeight;

    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    unsigned long outsize = 0;
    unsigned char * outbuffer = nullptr;

    jpeg_mem_dest(&cinfo, &outbuffer, &outsize);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = frame_info.nWidth;

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &frame.frame[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    jpeg_buffer.resize(outsize);
    memcpy(jpeg_buffer.data(), outbuffer, outsize);

    free(outbuffer);
}

void raw_frame::copy_frame(u_char *data, MV_FRAME_OUT_INFO_EX *frame_info) {
    frame.resize(frame_info->nFrameLen);
    memcpy(frame.data(), data, frame_info->nFrameLen);
    memcpy(&this->frame_info, frame_info, sizeof(MV_FRAME_OUT_INFO_EX));
}
