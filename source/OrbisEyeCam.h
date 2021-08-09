/*
 *	OrbisEyeCam library
 *	Copyright (C) 2013,2014,2019 Antonio Jose Ramos Marquez (aka bigboss) @psxdev on twitter
 *
 *  Repository https://github.com/psxdev/OrbisEyeCam
 *
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
 */

#ifndef ORBISEYECAM_H
#define ORBISEYECAM_H
#include <string>
#include <cstdio>
#include <cstdlib>
#include <vector>

 // define shared_ptr in std

#if (defined( _MSC_VER ) && ( _MSC_VER >= 1600 )) || (__cplusplus >= 201103L)
#include <memory>
#else
#include <tr1/memory>
namespace std {
	using std::tr1::shared_ptr;
	using std::tr1::weak_ptr;
	using std::tr1::static_pointer_cast;
	using std::tr1::dynamic_pointer_cast;
	using std::tr1::const_pointer_cast;
	using std::tr1::enable_shared_from_this;
}
#endif

#include <Windows.h>
#include <winusb.h>
#include <usb.h>
#include <SetupAPI.h>

#ifndef __STDC_CONSTANT_MACROS
#  define __STDC_CONSTANT_MACROS
#endif

#include <stdint.h>

#define debug printf


// {932F61A9-6CF0-6FAF-8861-DA0D8B023C5F}
DEFINE_GUID(GUID_DEVINTERFACE_USBBOOT,
	0x932F61A9, 0x6CF0, 0x6FAF, 0x88, 0x61, 0xDA, 0x0D, 0x8B, 0x02, 0x3C, 0x5F);

// {88BAE032-5A81-49F0-BC3D-A4FF138216D6}
DEFINE_GUID(GUID_DEVCLASS_USBDEVICE,
	0x88bae032, 0x5a81, 0x49f0, 0xbc, 0x3d, 0xa4, 0xff, 0x13, 0x82, 0x16, 0xd6);

typedef struct OrbisEyeCamFrame
{
	BYTE* header;
	BYTE* audio;
	BYTE* videoFrame[2];
	BYTE* videoFrameHigh[2];
	BYTE* videoFrameMedium[2];
	BYTE* videoFrameLow[2];
	BYTE* mode; 

}OrbisEyeCamFrame;

namespace orbiseye {

	class OrbisEyeCam
	{
	public:
		typedef std::shared_ptr<OrbisEyeCam> OrbisEyeCamRef;

		static const uint16_t VENDOR_ID;
		static const uint16_t PRODUCT_ID;
		std::string  firmware_path;
		int rightflag;




		OrbisEyeCam(HANDLE device, bool firmware_check);
		~OrbisEyeCam();

		//
		static const std::vector<OrbisEyeCamRef>& getDevices(bool forceRefresh = false);
		bool updateDevices();
		void firmware_upload();



	private:
		bool controlTransferReturned;
		bool controlTransferError;
		//firmware flag default false
		bool firmwareisloaded;
		uint32_t frame_width;
		uint32_t frame_height;
		uint32_t frame_stride;
		uint32_t linesize;
		uint8_t frame_rate;
		uint8_t mode;
		OrbisEyeCamFrame eyeFrame;
		OrbisEyeCam(const OrbisEyeCam&);

		void getCurrentFirmwarePath();
		void release();

		bool is_streaming;


		std::shared_ptr<class UsbManager> mgrPtr;

		static bool devicesEnumerated;
		static std::vector<OrbisEyeCamRef> devices;



		//usb stuff
		HANDLE device_;
		WINUSB_INTERFACE_HANDLE handle_;
		WINUSB_INTERFACE_HANDLE base_handle_;

		OVERLAPPED control_transfer;

		uint8_t* control_transfer_buffer;

		//bool OrbisEyeCam::init(uint8_t initmode, uint8_t desiredFrameRate);


		int submitAndWait_controlTransfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t* buffer);

		void submit_controlTransfer(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t* buffer);

		static uint8_t* controlTransferStatus(DWORD x);

		bool open_usb();
		void close_usb();

	};
}// namespace


#endif