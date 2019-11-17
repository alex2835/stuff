#include <windows.h>
#include <array>
#include <vector>

bool running = true;

#include "render_state.cpp"
#include "geometry.cpp"
#include "draw.cpp"
#include "timer.cpp"
#include "mouse_input.cpp"
#include "shapes.cpp"


LRESULT CALLBACK win_callback(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lParam)
{
	LRESULT res = 0;

	switch (uMsg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
	{
		running = false;
	} break;

	case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		surface.width = rect.right - rect.left;
		surface.height = rect.bottom - rect.top;

		int size = surface.width * surface.height * sizeof(unsigned int);

		if (surface.memory) VirtualFree(surface.memory, 0, MEM_RELEASE);
		surface.memory = (uint32_t*)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		surface.bitmap_info.bmiHeader.biSize = sizeof(surface.bitmap_info.bmiHeader);
		surface.bitmap_info.bmiHeader.biWidth = surface.width;
		surface.bitmap_info.bmiHeader.biHeight = surface.height;
		surface.bitmap_info.bmiHeader.biPlanes = 1;
		surface.bitmap_info.bmiHeader.biBitCount = 32;
		surface.bitmap_info.bmiHeader.biCompression = BI_RGB;

	} break;

	default:
	{
		res = DefWindowProc(hwnd, uMsg, wparam, lParam);
	}
	}
	return res;
}



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPiv, LPSTR args, int someshit)
{
	// create window class 
	WNDCLASS window_class = {};
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpszClassName = "lighter";
	window_class.lpfnWndProc = win_callback;

	// reg window
	RegisterClass(&window_class);

	// create window
	HWND window = CreateWindow(window_class.lpszClassName, "lighter",  WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, 0, 0, hInst, 0);
	HDC hdc = GetDC(window);


	// shapes
	std::vector<Line> shapes;
	shapes.reserve(30);
	add_some_shapes(shapes);

	// ray
	Line ray(Vert2f(surface.width / 2, surface.height / 2), Vec2f(), 200);

	// mouse input
	Mouse_Input mouse;

	// timer
	Timer timer(true);
	while (running)
	{
		// Input
		MSG msg;
		while (PeekMessage(&msg, window, 0, 0, PM_REMOVE))
		{
			switch (msg.message)
			{
				case WM_MOUSEMOVE:
				{
					mouse.x = int(LOWORD(msg.lParam));
					mouse.y = surface.height - int(HIWORD(msg.lParam));
				}break;
				case WM_LBUTTONDOWN:
				{
					mouse.buttons[LBUTTON].change = !mouse.buttons[LBUTTON].is_dawn;
					mouse.buttons[LBUTTON].is_dawn = true;
				}break;
				case WM_LBUTTONUP:
				{
					mouse.buttons[LBUTTON].change = mouse.buttons[LBUTTON].is_dawn;
					mouse.buttons[LBUTTON].is_dawn = false;
				}break;
				default:
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		// Simulate -----------------------------------------------------------
		if (surface.width == 0 || surface.height == 0) continue; // window is minimazed
		

		// calturate the ray
		Vec2f dir(mouse.x - ray.pos.x, mouse.y - ray.pos.y);
		ray.dir = dir.normalize();

		// segment of shape: coef of lines intersection 0 < T2 < 1
		float T2 = 2000;
		// ray: T1 > 0
		float T1 = 680;

		for (Line line : shapes)
		{
			float new_T2 = (ray.dir.x * (line.pos.y - ray.pos.y) + ray.dir.y * (ray.pos.x - line.pos.x)) /
										(line.dir.x * ray.dir.y - line.dir.y * ray.dir.x);

			// clossest srgment
			if (new_T2 > 0 && new_T2 < line.lenght)
			{
				T2 = new_T2;
				float new_T1 = (line.pos.x + line.dir.x * T2 - ray.pos.x) / ray.dir.x;
				if (new_T1 < T1 && new_T1 > 0)
					T1 = new_T1;
			}
		}

		ray.lenght = T1;



		// Draw ---------------------------------------------------------------
		
		// clear screen
		draw_filled_rect(0, 0, surface.width, surface.height, Color(0, 0, 0));

		// draw shapes
		for (Line line: shapes)
			line.draw(Color(255, 255, 255));

		// draw mouse point
		draw_filled_circle(mouse.x, mouse.y, 5, Color(255, 0, 0));

		// draw ray
		ray.draw();
		// intersection point
		draw_filled_circle(ray.pos.x + ray.dir.x * ray.lenght, ray.pos.y + ray.dir.y * ray.lenght, 5, Color(255, 0, 0));


		// timer
		timer.update();

		// Render
		StretchDIBits(hdc, 0, 0, surface.width, surface.height, 0, 0, surface.width, surface.height, surface.memory, &surface.bitmap_info, DIB_RGB_COLORS, SRCCOPY);

	}

	return 0;
}