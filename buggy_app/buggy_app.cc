#include <string>
#include <sstream>

#if defined(__linux)
#include "client/linux/handler/exception_handler.h"
#include "http_upload.h"
#elif defined(__APPLE__)
#include "client/mac/handler/exception_handler.h"
#elif defined(WIN32)
#include "client/windows/handler/exception_handler.h"
#include <locale>
#include <codecvt>
#endif

using std::string;

// Build with -g
// Create the symbols file
//   dump_syms ./buggy_app > buggy_app.sym
// Upload to backtrace.io
//   curl -v --data-binary @buggy_app.sym 'https://openalpr.sp.backtrace.io:6098/post?format=symbols&token=a7b23a9186a69ee9d51eed93bce3992cc6e38c8514eb534b74c0ec64f41b331c'
// Remove debug info from binary
//   objcopy --strip-debug somelibrary.so


static bool
dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
    void* context, bool succeeded)
{

    printf("Dump path1: %s\n", descriptor.path());

    (void) context;
    if (succeeded == true) {
        std::map<string, string> parameters;
        std::map<string, string> files;
        std::string proxy_host;
        std::string proxy_userpasswd;

        // See above for the URL section for how to generate the proper url for your server
        std::string url("somewhere");

        // Add any attributes to the parameters map.
        // Attributes such as uname.sysname, uname.version, cpu.count are
        // extracted from minidump files automatically.
        parameters["product_name"] = "foo";
        parameters["version"] = "0.1.1";
        // parameters["uptime"] uptime_sec;

        files["upload_file_minidump"] = descriptor.path();

        std::string response, error;
        bool success = google_breakpad::HTTPUpload::SendRequest(url,
                                               parameters,
                                               files,
                                               proxy_host,
                                               proxy_userpasswd,
                                               "",
                                               &response,
                                               NULL,
                                               &error);
    }

    printf("Dump path2: %s\n", descriptor.path());
    return succeeded;
}

void setupBreakpad(const string& outputDirectory) {
	google_breakpad::ExceptionHandler *exception_handler;

#if defined(__linux)
    google_breakpad::MinidumpDescriptor descriptor(outputDirectory);
	exception_handler = new google_breakpad::ExceptionHandler(
		descriptor, /* minidump output directory */
		NULL,   /* filter */
		dumpCallback,   /* minidump callback */
		NULL,   /* callback_context */
		true, /* install_handler */
		-1
	);

#elif defined(__APPLE__)
	exception_handler = new google_breakpad::ExceptionHandler(
		outputDirectory, /* minidump output directory */
		0,   /* filter */
		0,   /* minidump callback */
		0,   /* callback_context */
		true, /* install_handler */
		0    /* port name, set to null so in-process dump generation is used. */
	);
#elif defined(WIN32)
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> strConv;
	exception_handler = new google_breakpad::ExceptionHandler(
		strConv.from_bytes(outputDirectory), /* minidump output directory */
		0,   /* filter */
		0,   /* minidump callback */
		0,   /* calback_context */
		google_breakpad::ExceptionHandler::HANDLER_ALL /* handler_types */
	);

	// call TerminateProcess() to prevent any further code from
	// executing once a minidump file has been written following a
	// crash.  See ticket #17814
	exception_handler->set_terminate_on_unhandled_exception(true);
#endif
}

// This variable is NOT used - it only exists to avoid
// the compiler to inline the function aBuggyFunction
// so we can have a full backtrace.
int avoidInlineFunction = 1;

void aBuggyFunction() {
	if (avoidInlineFunction == 2)
	{
		// It never uses this code path, it only exists to avoid
		// this function to be inlined.
		aBuggyFunction();
	}
	int* invalid_ptr = reinterpret_cast<int*>(0x42);
	*invalid_ptr = 0xdeadbeef;
}

int main(int, char**) {
	setupBreakpad(".");
	aBuggyFunction();
	return 0;
}
