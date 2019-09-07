/*
This code handles dynamic registration of a python function.

The approach is very much inspired by the setup in ExcelDNA. 
However, to support the more dynamic approach to registering which you would want from python, 
the idea is extended a bit, using a non-standard C++ feature.
So, when a python function is registered, a pythonFunctionAdapter object is created, 
which holds a reference to the python function name and a non-virtual method used to invoke the function.
For this to work from Excel, we need to store both a pointer to the object and the member function.
The Visual Studio calling conventions for non-virtual member functions are similar to calling free functions, 
except that the pointer to the object is in register ecx.

More concretley, when a python function is registered, an adapter object is created, and 
pointers to both the object and the member function are stored in thunksObjects and thunksMethods respectively.
The free function exposed to Excel (the expf functions created by macros below) then moves the object adress to ecx and jumps to member function.
*/

#include "typeConversion.h"
#include "pythonFunctionAdapter.h"
#include "functionRegistration.h"


// thunk tables 
extern "C"
{
	void* thunksObjects[10];
	void* thunksMethods[10];
}

void registerPythonFunction(const py::str& pyFunctionName, const py::str& argumentType)
{
	static int functionIndex = 0;

	const std::string functionName = pyFunctionName;
	const std::wstring functionName_wide = cast_string(functionName);

	const std::wstring exportName = L"f" + std::to_wstring(functionIndex);
	const std::wstring xllName = L"xll." + functionName_wide;

	BindTypes argumentTypeInternal = get_bind_type(argumentType);
	std::wstring argTypeXll = get_xll_type(argumentTypeInternal);

	// create function object and register it in thunks
	PythonFunctionAdapter* pythonFunction = new PythonFunctionAdapter(functionName, argumentTypeInternal);

	xll::LPOPER(__thiscall PythonFunctionAdapter:: * pFunc)(void*) = &PythonFunctionAdapter::fct;

	thunksMethods[functionIndex] = (void*&)pFunc;
	thunksObjects[functionIndex] = pythonFunction;


	// Information Excel needs to register add-in.
	xll::Args functionBuilder = xll::Function(XLL_LPOPER, exportName.c_str(), xllName.c_str())
		.Arg(argTypeXll.c_str(), L"x", L"input")
		.Category(L"XLL")
		.FunctionHelp(L"some help text");

	xll::AddIn function = xll::AddIn(functionBuilder);
	++functionIndex;
}

PYBIND11_EMBEDDED_MODULE(excelbind, m) {
	// `m` is a `py::module` which is used to bind functions and classes
	m.def("register", &registerPythonFunction);
}


#define expf(i) extern "C" __declspec(dllexport,naked)	void f##i(void) {  \
__asm mov ecx, thunksObjects + i * 4 \
__asm jmp thunksMethods + i * 4 \
} \


expf(0)
expf(1)
expf(2)
expf(3)
expf(4)
expf(5)
expf(6)
expf(7)
expf(8)
expf(9)
