#include <Windows.h>
#include <string>
#include <Psapi.h>
#include <sstream>
#include <DbgHelp.h>
#include <commctrl.h>
#include <iomanip>
#include "EncodingConvert.h"
#include <mutex>
#include "CQPluginLoader.h"
using namespace std;

// 错误处理
#pragma comment(lib,"Dbghelp.lib")


typedef unsigned int exception_code_t;

const char* opDescription(const ULONG opcode)
{
	switch (opcode) {
	case 0: return "read";
	case 1: return "write";
	case 8: return "user-mode data execution prevention (DEP) violation";
	default: return "unknown";
	}
}
#define EXCEPTION_UNCAUGHT_CXX_EXCEPTION 0xe06d7363

void getCxxExceptionInfo(std::ostringstream& oss, ULONG_PTR expInfo)
{
	__try
	{
		const char * info = reinterpret_cast<std::exception*>(expInfo)->what();
		oss << "CXX Exception Info: " << info;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}
}

std::string seDescription(const exception_code_t& code)
{
	switch (code) {
	case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
	case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
	case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
	case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
	case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
	case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
	case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
	case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
	case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
	case EXCEPTION_UNCAUGHT_CXX_EXCEPTION:   return "EXCEPTION_UNCAUGHT_CXX_EXCEPTION";
	default: return "UNKNOWN EXCEPTION " + std::to_string(code);
	}
}

std::string expInformation(struct _EXCEPTION_POINTERS* ep, bool has_exception_code = false, exception_code_t code = 0)
{
	HMODULE hm;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, 
		static_cast<LPCTSTR>(ep->ExceptionRecord->ExceptionAddress), &hm);
	MODULEINFO mi;
	GetModuleInformation(GetCurrentProcess(), hm, &mi, sizeof(mi));
	char fn[MAX_PATH];
	GetModuleFileNameExA(GetCurrentProcess(), hm, fn, MAX_PATH);

	std::ostringstream oss;
	oss << "SE " << (has_exception_code ? seDescription(code) : "") << " at address 0x" << std::hex << ep->ExceptionRecord->ExceptionAddress << std::dec
		<< " inside " << fn << " loaded at base address 0x" << std::hex << mi.lpBaseOfDll << "\n";

	if (has_exception_code && (
		code == EXCEPTION_ACCESS_VIOLATION ||
		code == EXCEPTION_IN_PAGE_ERROR)) {
		oss << "Invalid operation: " << opDescription(ep->ExceptionRecord->ExceptionInformation[0]) << " at address 0x" << std::hex << ep->ExceptionRecord->ExceptionInformation[1] << std::dec << "\n";
	}

	if (has_exception_code && code == EXCEPTION_IN_PAGE_ERROR) {
		oss << "Underlying NTSTATUS code that resulted in the exception " << ep->ExceptionRecord->ExceptionInformation[2] << "\n";
	}

	if (has_exception_code &&
		code == EXCEPTION_UNCAUGHT_CXX_EXCEPTION)
	{
		getCxxExceptionInfo(oss, ep->ExceptionRecord->ExceptionInformation[1]);
	}

	return oss.str();
}


std::string formatStack(CONTEXT* ctx) //Prints stack trace based on context record
{
	std::stringstream ret;
	const int MaxNameLen = 256;

	BOOL    result;
	HANDLE  process;
	HANDLE  thread;
	HMODULE hModule;

	STACKFRAME64        stack;
	ULONG               frame;
	DWORD64             displacement;

	DWORD disp;
	IMAGEHLP_LINE64* line;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	char module[MaxNameLen];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	memset(&stack, 0, sizeof(STACKFRAME64));

	process = GetCurrentProcess();
	thread = GetCurrentThread();
	displacement = 0;
#if !defined(_M_AMD64)
	stack.AddrPC.Offset = (*ctx).Eip;
	stack.AddrPC.Mode = AddrModeFlat;
	stack.AddrStack.Offset = (*ctx).Esp;
	stack.AddrStack.Mode = AddrModeFlat;
	stack.AddrFrame.Offset = (*ctx).Ebp;
	stack.AddrFrame.Mode = AddrModeFlat;
#endif

	for (frame = 0; ; frame++)
	{
		//get next call from stack
		result = StackWalk64
		(
#if defined(_M_AMD64)
			IMAGE_FILE_MACHINE_AMD64
#else
			IMAGE_FILE_MACHINE_I386
#endif
			,
			process,
			thread,
			&stack,
			ctx,
			NULL,
			SymFunctionTableAccess64,
			SymGetModuleBase64,
			NULL
		);

		if (!result) break;

		//get symbol name for address
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		hModule = NULL;
		lstrcpyA(module, "");
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)(stack.AddrPC.Offset), &hModule);

		if (SymFromAddr(process, (ULONG64)stack.AddrPC.Offset, &displacement, pSymbol))
		{
			line = (IMAGEHLP_LINE64*)malloc(sizeof(IMAGEHLP_LINE64));
			if (line)
			{
				line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				ret << "\tat " << pSymbol->Name;
				//try to get line
				if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, line))
				{
					ret << " in " << line->FileName << ": line: " << dec << line->LineNumber;
				}
			}
			ret << ", address 0x" << hex << (DWORD64)hModule << "+0x" << hex << pSymbol->Address - (DWORD64)hModule << "+0x" << hex << stack.AddrPC.Offset - pSymbol->Address;

			free(line);
			line = NULL;
		}
		else
		{
			ret << "\tat address 0x" << hex << (DWORD64)hModule << "+0x" << hex << stack.AddrPC.Offset - (DWORD64)hModule;
		}


		//at least print module name
		if (hModule != NULL)
		{
			if (GetModuleFileNameA(hModule, module, MaxNameLen))
			{
				ret << " in " << module;
			}
		}

		ret << endl;

	}
	return ret.str();
}

std::mutex HandlerMutex;

LONG WINAPI CQXQUnhandledExceptionFilter(
	LPEXCEPTION_POINTERS ExceptionInfo
)
{
	std::unique_lock lock(HandlerMutex);

	// 调试用-加载符号
	SymInitialize(GetCurrentProcess(), NULL, TRUE);
	SymSetOptions(SYMOPT_LOAD_LINES);

	INITCOMMONCONTROLSEX ex;
	ex.dwICC = ICC_STANDARD_CLASSES;
	ex.dwSize = sizeof(ex);
	InitCommonControlsEx(&ex);

	TASKDIALOGCONFIG config;
	config.cbSize = sizeof(config);
	config.hwndParent = nullptr;
	config.hInstance = nullptr;
	config.dwFlags = TDF_EXPAND_FOOTER_AREA | TDF_SIZE_TO_CONTENT | TDF_USE_COMMAND_LINKS;
	config.dwCommonButtons = 0;
	config.pszWindowTitle = L"CQXQ 错误处理";
	config.pszMainIcon = TD_ERROR_ICON;
	config.pszMainInstruction = L"CQXQ 运行中遇到错误, 这可能是CQXQ本身或是某个插件导致的";

	std::wstring expInfo = GB18030toUTF16(expInformation(ExceptionInfo, true, ExceptionInfo->ExceptionRecord->ExceptionCode));
	config.pszContent = expInfo.c_str();

	constexpr int NUM_OF_BUTTONS = 4;
	config.cButtons = NUM_OF_BUTTONS;

	TASKDIALOG_BUTTON buttons[NUM_OF_BUTTONS];
	constexpr int IGNORE_BUTTON = 101;
	constexpr int RELOAD_BUTTON = 102;
	constexpr int RELOAD_EXCEPT_ERROR_BUTTON = 103;
	constexpr int EXIT_BUTTON = 104;

	buttons[0].nButtonID = IGNORE_BUTTON;
	buttons[0].pszButtonText = L"忽略\nCQXQ将忽略此异常, 但是应用程序可能表现异常";
	buttons[1].nButtonID = RELOAD_BUTTON;
	buttons[1].pszButtonText = L"重载\nCQXQ将重载所有插件";
	buttons[2].nButtonID = RELOAD_EXCEPT_ERROR_BUTTON;
	buttons[2].pszButtonText = L"重载(禁用出错应用)\nCQXQ将重载所有插件, 但将禁用出错应用(别点这个还没写完)";
	buttons[3].nButtonID = EXIT_BUTTON;
	buttons[3].pszButtonText = L"退出\n程序将会退出";

	config.pButtons = buttons;
	config.nDefaultButton = IGNORE_BUTTON;
	config.cRadioButtons = 0;
	config.pRadioButtons = nullptr;
	config.nDefaultRadioButton = 0;
	config.pszVerificationText = nullptr;
	std::wstring stkInfo = GB18030toUTF16(formatStack(ExceptionInfo->ContextRecord));
	config.pszExpandedInformation = stkInfo.c_str();
	config.pszExpandedControlText = L"隐藏";
	config.pszCollapsedControlText = L"详细信息";
	config.pszFooterIcon = nullptr;
	config.pszFooter = nullptr;
	config.pfCallback = nullptr;
	config.lpCallbackData = (LONG_PTR)nullptr;
	config.cxWidth = 0;

	int pnButton = 0;
	TaskDialogIndirect(&config, &pnButton, nullptr, nullptr);

	if (pnButton == EXIT_BUTTON)
	{
		SymCleanup(GetCurrentProcess());
		exit(EXIT_FAILURE);
	}
	else if (pnButton == RELOAD_BUTTON)
	{
		reloadAllCQPlugin();
	}
	SymCleanup(GetCurrentProcess());
	return EXCEPTION_EXECUTE_HANDLER;
}