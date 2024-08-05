#include <implus/host.hpp>

namespace Renderer {
void SetupHints();

void SetupInstance(ImPlus::Host::Window&);

void SetupWindow(ImPlus::Host::Window&);

void SetupImplementation(ImPlus::Host::Window&);

void ShutdownImplementation();

void ShutdownInstance();

void InvalidateDeviceObjects();

void NewFrame(ImPlus::Host::Window&);

void PrepareViewport(ImPlus::Host::Window&);

void RenderDrawData();

void SwapBuffers(ImPlus::Host::Window&);

} // namespace Renderer