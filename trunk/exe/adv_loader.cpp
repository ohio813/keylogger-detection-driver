// adv_loader.cpp : Defines the entry point for the console application.
// code adapted from www.sysinternals.com on-demand driver loading code
// --------------------------------------------------------------------
// brought to you by ROOTKIT.COM
// --------------------------------------------------------------------
//
// Modified by Christopher Wood
//
// Changes:
//    - no more command line arguments, operates based on user input.
//

//TODO: implement DeviceIoControl codes for triggering/untriggering keyboard and system monitoring!

#include <windows.h>
#include <process.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

string driverName;
string serviceName;
string serviceDisplayName;
string pathName;
LPCSTR pDriverName;
LPCSTR pServiceName;
LPCSTR pServiceDisplayName;
LPCSTR pPathName;

void GetInfo();

//
// Load the driver
//
void load(LPCSTR driverName, LPCSTR serviceName, LPCSTR serviceDisplayName, LPCSTR pathName)
{
	printf("Registering Driver.\n");
		
	SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!sh) 
	{
		printf("error OpenSCManager\n");
		//exit(1);
	}

	SC_HANDLE rh = CreateService(
		sh, 
		serviceName,
		serviceDisplayName,
		SERVICE_ALL_ACCESS, 
		SERVICE_KERNEL_DRIVER, 
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL, 
		pathName,
		NULL, 
		NULL, 
		NULL, 
		NULL, 
		NULL);

	printf("Service creation attempt complete, checking if service currently exists.\n");

	if(!rh) 
	{
		if (GetLastError() == ERROR_SERVICE_EXISTS) 
		{
			// service exists
			printf("Service already exists.\n");
			rh = OpenService(sh, 
							serviceName,
							SERVICE_ALL_ACCESS);

			if(!rh)
			{
				printf("WARNING: error OpenService\n");
				CloseServiceHandle(sh);
				//exit(1);
			}
		} 
		else 
		{
			printf("WARNING: error CreateService\n");
			CloseServiceHandle(sh);
			//exit(1);
		}
	}
}

//
// Unload the driver
//
void unload(LPCSTR serviceName)
{
	SERVICE_STATUS ss;

	printf("Unloading Driver.\n");
		
	SC_HANDLE sh = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(!sh) 
	{
		printf("WARNING: error OpenSCManager\n");
		//exit(1);
	}
	SC_HANDLE rh = OpenService(	
						sh, 
						serviceName,
						SERVICE_ALL_ACCESS);

	printf("Service open attempt complete.\n");

	if(!rh) 
	{
		printf("WARNING: error with OpenService, did you load the driver yet?\n");
		CloseServiceHandle(sh);
		//exit(1);
	}

	if(!ControlService(rh, SERVICE_CONTROL_STOP, &ss))
	{
		printf("WARNING: Could not stop service.\n");
	}

	if (!DeleteService(rh)) 
	{
		printf("WARNING: Could not delete service.\n");
	}

	CloseServiceHandle(rh);
	CloseServiceHandle(sh);
}

//
// Main function
//
int main()
{
	int choice;
	bool cont = true;

	// print the header and comments
	printf("adv_loader.cpp : Defines the entry point for the console application.\n");
	printf("Code adapted from www.sysinternals.com on-demand driver loading code.\n");
	printf("--------------------------------------------------------------------\n");
	printf("Brought to you by ROOTKIT.com -- Modified by Christopher Wood\n");
	printf("--------------------------------------------------------------------\n\n");

	// get driver info.
	GetInfo();

	printf("Information retrieval complete.\n");
	do
	{
		printf("Please select the next option:\n\t1.) Load the driver\n\t2.) Unload the driver\n\t3.) Reset driver info.\n\t4.) Exit \n");
		scanf("%d", &choice);
		switch (choice)
		{
		case 1: // load
			load(pDriverName, pServiceName, pServiceDisplayName, pPathName); //fix changes in load before using this
			break;
		case 2: // unload
			unload(pServiceName); //fix changes in unload before using this
			break;
		case 3: // reset
			GetInfo();
			printf("Information retrieval complete.\n");
			break;
		case 4: // exit
			printf("Exiting now.\n");
			cont = false;
			break;
		default: // invalid
			printf("Invalid selection. Try again.\n");
			break;
		}
	} while (cont == true);
	return 0;
}

//
// Get driver information...
//
void GetInfo()
{
	// prompt the user for information about the driver
	printf("Driver name (eg. driver.sys): \n");
	getline(cin, driverName);
	printf("Driver location (eg. C:\\): \n");
	getline(cin, pathName);
	printf("Driver Service Name (eg. sample_driver): \n");
	getline(cin, serviceName);
	printf("Driver Service Display Name (eg. Sample Driver): \n");
	getline(cin, serviceDisplayName);
	cin.clear();

	// convert the strings to LPCSTR types (old c strings)
	pDriverName = driverName.c_str();
	pPathName = pathName.c_str();
	pServiceName = serviceName.c_str();
	pServiceDisplayName = serviceDisplayName.c_str();
}

