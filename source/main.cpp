#include "pch.h"
#include "OrbisEyeCam.h"

#include <stdio.h>
#include <direct.h>

using namespace orbiseye;

std::string getCurrentFirmwarePath()
{
	char buff[FILENAME_MAX];
	_getcwd(buff, FILENAME_MAX);
	std::string currentDir(buff);
	
	 currentDir.append(std::string("\\firmware.bin"));
	 return currentDir;
}

LONG __cdecl
_tmain(
    LONG     Argc,
    LPTSTR * Argv
    )
/*++

Routine description:

    Firmware loader for Usb Boot device using WinUSB

--*/
{
    

    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Argv);

	orbiseye::OrbisEyeCam::OrbisEyeCamRef eye;


	std::vector<OrbisEyeCam::OrbisEyeCamRef> devices(OrbisEyeCam::getDevices());
	if (devices.size() == 1)
	{
		eye = devices.at(0);

		//eye->firmware_path = "C:\\pathtoyourfirmware\\firmware.bin"; or use getCurrentFirmwarePath get .exe current path and add firmware.bin
		eye->firmware_path = getCurrentFirmwarePath();


		eye->firmware_upload();

	}
	else
	{
		debug("Usb Boot device not found...\n");
	}


	return 0;
   
}
