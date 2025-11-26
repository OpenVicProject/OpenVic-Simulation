#pragma once

#include <cstdint>
#include <string_view>

namespace OpenVic {
	enum class Error : uint8_t {
		OK, // Being 0 means failure can be checked with "if (Error)"
		FAILED, // Generic failure
		UNAVAILABLE, // Unsupported/unavailable behavior
		UNCONFIGURED, // Not properly setup
		UNAUTHORIZED,
		FILE_NOT_FOUND,
		FILE_BAD_PATH,
		FILE_NO_PERMISSION,
		FILE_ALREADY_IN_USE,
		FILE_CANT_OPEN,
		FILE_CAN_WRITE,
		FILE_CANT_READ,
		LOCKED, // Attempted to modify locked object
		TIMEOUT,
		CANT_CONNECT,
		CANT_RESOLVE,
		CONNECTION_ERROR,
		INVALID_DATA,
		INVALID_PARAMETER,
		ALREADY_EXISTS,
		DOES_NOT_EXIST,
		BUSY,
		SKIP,
		BUG, // A bug in the software, should be reported
		MAX
	};

	static constexpr std::string_view as_string(Error err) {
		switch (err) {
			using enum Error;
		case OK:				  return "OK";
		case FAILED:			  return "Failed";
		case UNAVAILABLE:		  return "Unavailable";
		case UNCONFIGURED:		  return "Unconfigured";
		case UNAUTHORIZED:		  return "Unauthorized";
		case FILE_NOT_FOUND:	  return "File not found";
		case FILE_BAD_PATH:		  return "File: Bad path";
		case FILE_NO_PERMISSION:  return "File: Permission Denied";
		case FILE_ALREADY_IN_USE: return "File already in use";
		case FILE_CANT_OPEN:	  return "Can't open file";
		case FILE_CAN_WRITE:	  return "Can't write file";
		case FILE_CANT_READ:	  return "Can't read file";
		case LOCKED:			  return "Locked";
		case TIMEOUT:			  return "Timeout";
		case CANT_CONNECT:		  return "Can't connect";
		case CANT_RESOLVE:		  return "Can't resolve";
		case CONNECTION_ERROR:	  return "Connection error";
		case INVALID_DATA:		  return "Invalid data";
		case INVALID_PARAMETER:	  return "Invalid parameter";
		case ALREADY_EXISTS:	  return "Already exists";
		case DOES_NOT_EXIST:	  return "Does not exist";
		case BUSY:				  return "Busy";
		case SKIP:				  return "Skip";
		case BUG:				  return "Bug";
		case MAX:				  return "";
		}
	}
}
