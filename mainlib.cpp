//
// Created by wuyua on 11/27/2020.
//
#include "MvCameraControl.h"
#include <cstring>
#include <iostream>
#include <thread>
#include <mutex>


void __stdcall ImageCallBackEx(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    if (pFrameInfo)
    {

        auto* pDataForSaveImage = new u_char[pFrameInfo->nWidth * pFrameInfo->nHeight * 4 + 2048];
        // fill in the parameters of save image
        MV_SAVE_IMAGE_PARAM_EX stSaveParam;
        memset(&stSaveParam, 0, sizeof(MV_SAVE_IMAGE_PARAM_EX));
        // 从上到下依次是：输出图片格式，输入数据的像素格式，提供的输出缓冲区大小，图像宽，
        // 图像高，输入数据缓存，输出图片缓存，JPG编码质量
        // Top to bottom are：
        stSaveParam.enImageType = MV_Image_Jpeg;
        stSaveParam.enPixelType = pFrameInfo->enPixelType;
        stSaveParam.nBufferSize = pFrameInfo->nWidth * pFrameInfo->nHeight * 4 + 2048;
        stSaveParam.nWidth      = pFrameInfo->nWidth;
        stSaveParam.nHeight     = pFrameInfo->nHeight;
        stSaveParam.pData       = pData;
        stSaveParam.nDataLen    = pFrameInfo->nFrameLen;
        stSaveParam.pImageBuffer = pDataForSaveImage;
        stSaveParam.nJpgQuality = 100;
        int nRet = MV_CC_SaveImageEx(&stSaveParam);
        if(MV_OK != nRet)
        {
            std::cout <<"failed in MV_CC_SaveImage" << std::endl;
            return;
        }
        FILE* fp = fopen("image.jpg", "wb");
        if (nullptr == fp)
        {
            return;
        }
        fwrite(pDataForSaveImage, 1, stSaveParam.nImageLen, fp);
        fclose(fp);
        printf("save image succeed\n");

        printf("GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n",
               pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->nFrameNum);
        delete[] pDataForSaveImage;
    }
}


int test() {
    uint version = MV_CC_GetSDKVersion();

    std::cout << "Version=" << version << std::endl;

    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    // 枚举设备
    // enum device
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_EnumDevices fail! nRet" << std::endl;
        return nRet;
    }


    if (stDeviceList.nDeviceNum > 0)
    {
        for (int i = 0; i < stDeviceList.nDeviceNum; i++)
        {
            std::cout << "[device " << i << "]: "<< std::endl;
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            if (NULL == pDeviceInfo)
            {
                break;
            }
            std::cout <<  pDeviceInfo->SpecialInfo.stUsb3VInfo.chModelName << std::endl;

        }
    }
    else
    {
        std::cout <<"Find No Devices!" << std::endl;
        return 0;
    }

    // Open device
    void* handle = nullptr;

    nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[0]);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_CreateHandle fail!" << std::endl;
        return nRet;
    }


    nRet = MV_CC_OpenDevice(handle);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_OpenDevice fail!" << std::endl;
        return nRet;
    }


    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetEnumValue TriggerMode fail!" << std::endl;
        return nRet;
    }

    nRet = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", 1.0);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetFloatValue AcquisitionFrameRate fail!" << std::endl;
        return nRet;
    }

    // register image callback
    nRet = MV_CC_RegisterImageCallBackEx(handle, ImageCallBackEx, handle);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_RegisterImageCallBackEx fail!" << std::endl;
        return nRet;
    }

    // start grab image
    nRet = MV_CC_StartGrabbing(handle);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_StartGrabbing fail!" << std::endl;
        return nRet;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // end grab image
    nRet = MV_CC_StopGrabbing(handle);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_StopGrabbing fail!" << std::endl;
        return nRet;
    }
    // 关闭设备
    // close device
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_CloseDevice fail!" << std::endl;
        return nRet;
    }

    // Finalize
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_DestroyHandle TriggerMode fail!" << std::endl;
        return nRet;
    }



    return 0;
}