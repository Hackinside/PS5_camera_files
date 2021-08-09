/*
 *	OrbisEyeCam library
 *	Copyright (C) 2013,2014,2019 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 *
 *  Repository https://github.com/psxdev/OrbisEyeCam

 *  This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public Licence as published by
 *	the Free Software Foundation; either version 2 of the Licence, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public Licence for more details.
 *
 *	You should have received a copy of the GNU General Public Licence
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 *  If you redistribute this file in source form, modified or unmodified, you
 *  may:
 *  1) Leave this header intact and distribute it under the same terms,
 *  accompanying it with the GPL20 or later files, or
 *  In all cases you must keep the copyright notice intact
 *
 *  Binary distributions must follow the binary distribution requirements of
 *  license.
 *
 */
#include "OrbisEyeCam.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <Winusb.h>
#include <windows.h>
#include <SetupAPI.h>

#define CHUNK_SIZE 512
#define ISOCH_TRANSFER_COUNT 8

namespace orbiseye
{

	class UsbManager
	{
	public:
		UsbManager();
		~UsbManager();

		static std::shared_ptr<UsbManager>  instance();
		static WINUSB_INTERFACE_HANDLE usbContext() { return instance()->usb_context; }
		static int listDevices(std::vector<OrbisEyeCam::OrbisEyeCamRef>& list, const GUID* device_interface, bool firmware_check);
		static bool handle_events();

		static std::shared_ptr<UsbManager>  sInstance;

	private:
		WINUSB_INTERFACE_HANDLE usb_context;
		std::vector<OVERLAPPED> control_transfers;

		UsbManager(const UsbManager&);
	};

	std::shared_ptr<UsbManager> UsbManager::sInstance;

	UsbManager::UsbManager()
	{
		//libusb_init(&usb_context);
		//libusb_set_debug(usb_context, 3);
	}

	UsbManager::~UsbManager()
	{
		//debug("UsbManager destructor called\n");
		//libusb_exit(usb_context);
		//  libusb_exit(NULL);

	}

	std::shared_ptr<UsbManager> UsbManager::instance()
	{
		if (!sInstance) {
			sInstance = std::shared_ptr<UsbManager>(new UsbManager);
		}
		return sInstance;
	}

	bool UsbManager::handle_events()
	{
		// TODO: Async control transfers
		/*DWORD transferred = 0;
		if (WinUsb_GetOverlappedResult(instance()->usb_context, &instance()->control_transfer, &transferred, FALSE))
			cam->ct_done()*/

		return true;
	}
	int UsbManager::listDevices(std::vector<OrbisEyeCam::OrbisEyeCamRef>& list, const GUID* device_interface, bool firmware_check)
	{
		SP_DEVINFO_DATA dev;
		dev.cbSize = sizeof(SP_DEVINFO_DATA);
		SP_DEVICE_INTERFACE_DATA intf;
		intf.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		HDEVINFO devs;
		int i = 0;
		int cnt;

		devs = SetupDiGetClassDevs(device_interface, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

		if (devs == INVALID_HANDLE_VALUE)
			debug("Error Device scan\n");

		cnt = 0;
		if (SetupDiEnumDeviceInfo(devs, 0, &dev))
		{
		
			while (SetupDiEnumDeviceInterfaces(devs, &dev, device_interface, i, &intf))
			{
				i++;
				// get string size
				DWORD size = 0;
				SetupDiGetDeviceInterfaceDetail(devs, &intf, nullptr, 0, &size, nullptr);
				PSP_DEVICE_INTERFACE_DETAIL_DATA data = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(size);
				data->cbSize = sizeof(PSP_DEVICE_INTERFACE_DETAIL_DATA);
				if (!SetupDiGetDeviceInterfaceDetail(devs, &intf, data, size, nullptr, nullptr))
					continue;

				HANDLE handle = CreateFile(data->DevicePath,
					GENERIC_WRITE | GENERIC_READ,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					NULL);
				free(data);

				if (handle != INVALID_HANDLE_VALUE)
				{
					list.push_back(OrbisEyeCam::OrbisEyeCamRef(new OrbisEyeCam(handle, firmware_check)));
					cnt++;
				}
			}
		}

		SetupDiDestroyDeviceInfoList(devs);

		return cnt;
	}

	// OrbisEyeCam

	bool OrbisEyeCam::devicesEnumerated = false;
	std::vector<OrbisEyeCam::OrbisEyeCamRef> OrbisEyeCam::devices;

	const std::vector<OrbisEyeCam::OrbisEyeCamRef>& OrbisEyeCam::getDevices(bool forceRefresh)
	{
		if (devicesEnumerated && (!forceRefresh))
			return devices;

		devices.clear();

		

		if (UsbManager::instance()->listDevices(devices, &GUID_DEVINTERFACE_USBBOOT, false) > 0)
			std::cout << "Found OrbisEyeCam UsbBoot mode" << std::endl;

		devicesEnumerated = true;
		return devices;
	}

	bool OrbisEyeCam::updateDevices()
	{
		//if (urb)
			//urb->handle_events(device_);
		return true;
	}


	OrbisEyeCam::OrbisEyeCam(HANDLE device, bool firmware_check)
		: control_transfer()
	{
		// default controls

		handle_ = NULL;
		base_handle_ = NULL;

		is_streaming = false;
		rightflag = 0;
		device_ = device;
		mgrPtr = UsbManager::instance();
		//urb = std::shared_ptr<URBDesc>( new URBDesc() );


		// create a usb control trasfer packet. This is used to send commands to the device.
		control_transfer.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		control_transfer_buffer = new uint8_t[72];
		//flag to check if firmware is loaded
		firmwareisloaded = firmware_check;

	}

	OrbisEyeCam::~OrbisEyeCam()
	{
		debug("OrbisEyeCam destructor\n");
		//stop();
		release();
	}
	
	void OrbisEyeCam::release()
	{
		debug("OrbisEyeCam release called\n");

		if (handle_ != NULL)
			close_usb();
		if (control_transfer_buffer) free(control_transfer_buffer);
		
	}



	
	
	bool OrbisEyeCam::open_usb()
	{
		
		return true;
	}

	void OrbisEyeCam::close_usb()
	{
		debug("closing device\n");
		WinUsb_Free(handle_);
		WinUsb_Free(base_handle_);
		CloseHandle(device_);

		handle_ = NULL;
		device_ = NULL;
		debug("device closed\n");
	}


	/*
	 * Control error messages for usb transfers
	 */
	uint8_t* OrbisEyeCam::controlTransferStatus(DWORD x)
	{
		switch (x)
		{
		case NO_ERROR:
			return (uint8_t*)"Transfer completed without error";
			//case LIBUSB_TRANSFER_ERROR:
			//    return (uint8_t *)"Transfer failed";
		case ERROR_TIMEOUT:
			return (uint8_t*)"Transfer timed out";
		case ERROR_OPERATION_ABORTED:
			return (uint8_t*)"Transfer was cancelled";
		case ERROR_GEN_FAILURE:
			return (uint8_t*)"Stall";
		case ERROR_HANDLE_EOF:
			return (uint8_t*)"Device was disconnected";
			//case LIBUSB_TRANSFER_OVERFLOW:
			//    return (uint8_t *)"Transfer Overflow";
		default:
			return (uint8_t*)"Unknown error jur jur";
		}
	}

	/*
	 * Called when control transfer is done
	 */
	 /*void  LIBUSB_CALL PS4EYECam::ct_done(struct libusb_transfer * transfer) {
		 PS4EYECam * cam = (PS4EYECam *) transfer->user_data;
		 if (transfer->status != ERROR_OPERATION_ABORTED)
		 {
			 debug("Control transfer error: %s\n", controlTransferStatus(transfer->status) );
			 cam->controlTransferError = true;
		 }
		 if (cam->control_wLength != transfer->actual_length) {
			 debug("phase 2 read failed received %d bytes instead of %d\n",transfer->actual_length ,cam->control_wLength);
			 cam->controlTransferError = true;
		 }
		 cam->controlTransferReturned = true;
	 }*/

	 /*
	  * Submit a control transfer to the usb system and wait until this packet has
	  * been processed.
	  */
	int OrbisEyeCam::submitAndWait_controlTransfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t* buffer)
	{
		controlTransferReturned = false;
		controlTransferError = false;
		DWORD actual_length = 0;
		WINUSB_SETUP_PACKET setup = { bmRequestType, bRequest, wValue, wIndex, wLength };
		BOOL status = WinUsb_ControlTransfer(handle_, setup, buffer, wLength, &actual_length, nullptr);

		if (!status)
		{
			debug("Control transfer error: %s\n", controlTransferStatus(GetLastError()));
			controlTransferError = true;
		}
		if (wLength != actual_length)
		{
			debug("phase 2 read failed received %d bytes instead of %d\n", actual_length, wLength);
			controlTransferError = true;
		}
		controlTransferReturned = true;
		return actual_length;
	}

	/*
	 * Submit a control transfer to the usb system and continue with the program flow, i.e.
	 * do not wait until the packet has been processed.
	 */
	void OrbisEyeCam::submit_controlTransfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t* buffer)
	{
		WINUSB_SETUP_PACKET setup = { bmRequestType, bRequest, wValue, wIndex, wLength };
		WinUsb_ControlTransfer(handle_, setup, buffer, wLength, nullptr, &control_transfer);
	}


	void OrbisEyeCam::firmware_upload()
	{
		if (!firmwareisloaded)
		{
			if (!WinUsb_Initialize(device_, &handle_))
			{
				std::cout << "ov580 Camera boot mode not found..." << std::endl;
				exit(0);
			}
			//cout <<"ov580 Camera boot mode found..." << endl;

			std::cout << "Uploading firmware to ov580 camera..." << std::endl;

			uint8_t chunk[CHUNK_SIZE];

			std::ifstream firmware(firmware_path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

			if (firmware.is_open())
			{
				uint32_t length = (uint32_t)firmware.tellg();
				firmware.seekg(0, std::ios::beg);

				uint16_t index = 0x14;
				uint16_t value = 0;

				for (uint32_t pos = 0; pos < length; pos += CHUNK_SIZE)
				{
					uint16_t size = (CHUNK_SIZE > (length - pos) ? (uint16_t)(length - pos) : CHUNK_SIZE);
					firmware.read((char*)chunk, size);
					submitAndWait_controlTransfer(0x40, 0x0, value, index, size, chunk);
					if (((uint32_t)value + size) > 0xFFFF) index += 1;

					value += size;
				}
				firmware.close();

				chunk[0] = 0x5b;
				submitAndWait_controlTransfer(0x40, 0x0, 0x2200, 0x8018, 1, chunk);

				std::cout << "Firmware uploaded..." << std::endl;
			}
			else
			{
				std::cout << "Unable to open firmware.bin!" << std::endl;
				std::cout << "Expected path for firmware:"+firmware_path << std::endl;
			}
			
		}
		else
		{
			std::cout << "Firmware already loaded..." << std::endl;
		}
	}



}