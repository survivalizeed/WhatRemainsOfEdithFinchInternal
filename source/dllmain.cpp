#include "pch.h"
#include "include.h"

#include <map>
#include <thread>
#include <sstream>

namespace intern {
	void patch(void* addr, std::vector<BYTE> bytes) {
		DWORD oldProtect;
		VirtualProtect(addr, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
		for (int i = 0; i < bytes.size(); ++i) {
			*((BYTE*)addr + i) = bytes[i];
		}
		VirtualProtect(addr, bytes.size(), oldProtect, &oldProtect);
	}

	bool isBadReadPtr(void* p)
	{
		MEMORY_BASIC_INFORMATION mbi = { 0 };
		if (::VirtualQuery(p, &mbi, sizeof(mbi)))
		{
			DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
			bool b = !(mbi.Protect & mask);
			if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) b = true;
			return b;
		}
		return true;
	}

	uintptr_t calcAddN(uintptr_t ptr, std::vector<unsigned int> offsets)
	{
		uintptr_t addr = ptr;		
		for (unsigned int i = 0; i < offsets.size(); ++i)
		{
			addr = *(uintptr_t*)addr;
			addr += offsets[i];
		}
		return addr;
	}

	uintptr_t calcAddS(uintptr_t ptr, std::vector<unsigned int> offsets, bool& valid)
	{
		uintptr_t addr = ptr;
		for (unsigned int i = 0; i < offsets.size() - 1; ++i) {
			if (isBadReadPtr((void*)(addr + offsets[i]))) {
				valid = false;
				return 0;
			}
			addr = *(uintptr_t*)(addr + offsets[i]);
		}
		valid = true;
		return addr + offsets[offsets.size() - 1];
	}

}

void refresh(ScreenInteractive& screen) {
	bool toggle = false;
	for (;;) {
		if (GetForegroundWindow() != GetConsoleWindow())
			screen.PostEvent(Event::Custom);
		if (input(VK_MULTIPLY)) {
			if (!toggle)
				ShowWindow(GetConsoleWindow(), SW_HIDE);
			else
				ShowWindow(GetConsoleWindow(), SW_RESTORE);
			toggle = !toggle;
			Sleep(300);
		}
		Sleep(1);
	}
}

template<typename T>
struct Vec3 {
	T x, y, z;
};

void doFly(std::map<std::string, bool>& states, Vec3<float>*& position, Vec3<float>*& rotation, 
	uintptr_t*& nullAddress1, uintptr_t& positionAddress, bool& run) {
	bool once = false;
	uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
	float increment = 1.f;
	constexpr float pi = 3.141592f;

	for (;;) {
		if (!run) continue;
		if (!states["fly"]) {
			if (once) {
				*nullAddress1 = positionAddress;
				once = false;
			}
			continue;
		}
		else if (!once) {
			if (nullAddress1 == NULL) {
				states["fly"] = false;
				once = false;
				continue;
			}
			*nullAddress1 = 0;
			once = true;
		}
		if (input(VK_SHIFT))
			increment = 3;
		else
			increment = 1;
		if (input('W')) {
			position->x += cosf(rotation->y * pi / 180) * 0.8f * increment;
			position->y += sinf(rotation->y * pi / 180) * 0.8f * increment;
			position->z += sinf(rotation->x * pi / 180) * 0.8f * increment;
		}
		if (input('S')) {
			position->x += -cosf(rotation->y * pi / 180) * 0.8f * increment;
			position->y += -sinf(rotation->y * pi / 180) * 0.8f * increment;
			position->z += -sinf(rotation->x * pi / 180) * 0.8f * increment;
		}
		if (input('A')) {
			position->x += -cosf((rotation->y + 90.f) * pi / 180) * 0.8f * increment;
			position->y += -sinf((rotation->y + 90.f) * pi / 180) * 0.8f * increment;
		}
		if (input('D')) {
			position->x += -cosf((rotation->y + 270.f) * pi / 180) * 0.8f * increment;
			position->y += -sinf((rotation->y + 270.f) * pi / 180) * 0.8f * increment;
		}
		Sleep(1);
	}
}

void initVariables(Vec3<float>*& position, Vec3<float>*& rotation, uintptr_t*& nullAddress1, uintptr_t& positionAddress, bool& run) {
	static Vec3<float> defPosition{ 1.f, 1.f, 1.f };
	static Vec3<float> defRotation{ 1.f, 1.f, 1.f };
	uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
	bool once = false;
	for (;;) {
		bool valid = false;
		if(!intern::isBadReadPtr((void*)(base + 0x02AF0530)))
			position = (Vec3<float>*)intern::calcAddS(*(uintptr_t*)(base + 0x02AF0530), { 0x30, 0x70, 0x3B8, 0x638, 0xA8, 0x160, 0x140 }, valid);
		if (!valid)
			position = &defPosition;
		if (!intern::isBadReadPtr((void*)(base + 0x02A95A30)))
			rotation = (Vec3<float>*)intern::calcAddS(*(uintptr_t*)(base + 0x02A95A30), { 0x0, 0x20, 0x5A8, 0xA8, 0x800, 0xC8, 0x18C }, valid);
		if (!valid)
			rotation = &defRotation;
		if (!intern::isBadReadPtr((void*)(base + 0x02AF0530)))
			nullAddress1 = (uintptr_t*)intern::calcAddS(*(uintptr_t*)(base + 0x02AF0530), { 0x30, 0x3B8, 0x590, 0x5A8, 0x78, 0x118, 0xC8 }, valid);
		if (!valid) {
			nullAddress1 = NULL;
			positionAddress = 0;
		}
		else if (!once) {
			positionAddress = *nullAddress1;
			once = true;
		}
	}
}


DWORD WINAPI MainThread(LPVOID param) {
	using namespace std;

	bool run = true;

	std::thread refreshT, doFlyT, initVariablesT;

	Vec3<float>* position = nullptr;
	Vec3<float>* rotation = nullptr;
	uintptr_t* nullAddress1 = nullptr;
	uintptr_t positionAddress = 0;

	initVariablesT = thread(initVariables, ref(position), ref(rotation), ref(nullAddress1), ref(positionAddress), ref(run));

	Sleep(1000);

	uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
	stringstream stream;
	stream << hex << base;
	string baseStr = stream.str();

	console::createNewConsole(1024);

	HWND hwnd = GetConsoleWindow();
	MoveWindow(hwnd, 0, 0, 420, 300, false);	// Size
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowLong(hwnd, GWL_STYLE, 0);
	ShowWindow(hwnd, SW_SHOW);
	SetParent(GetConsoleWindow(), FindWindowA(NULL, "What Remains of Edith Finch"));

	map<std::string, bool> states;
	auto container = Container::Vertical({});

	states["fly"] = false;
	container->Add(Checkbox("Fly", &states["fly"]));

	auto renderer = Renderer(container, [&] {
		return vbox({
				vbox({
					text("survivalizeed's WROEF internal") | color(Color::Color(209, 202, 0)),
					hbox({
						vbox({
							text("X: " + to_string(position->x)),
							text("Y: " + to_string(position->y)),
							text("Z: " + to_string(position->z)),
						}) | border | size(WIDTH, LESS_THAN, 40),
						vbox({
							text("Module Base: " + baseStr),
							text("Pitch: " + to_string(rotation->x)),
							text("Yaw: " + to_string(rotation->y)),
						}) | border | size(WIDTH, LESS_THAN, 40),
					}),
				}) | border,
			    container->Render() | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 15) | border,
			}) | border;
		});

	auto screen = ScreenInteractive::FitComponent();
	refreshT = thread(refresh, ref(screen));
	doFlyT = thread(doFly, ref(states), ref(position), ref(rotation), ref(nullAddress1), ref(positionAddress), ref(run));
	screen.Loop(renderer);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, NULL, MainThread, hModule, NULL, NULL);
		break;
	default:
		break;
	}
	return TRUE;
}
