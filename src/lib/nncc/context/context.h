#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/thread.h>

#include <nncc/context/event.h>
#include <nncc/context/hid.h>
#include <nncc/context/glfw_utils.h>

namespace nncc::context {

class Context {
public:
    Context() : glfw_message_queue_(&allocator_), event_queue_(&allocator_) {}

    static Context& Get() {
        static Context instance;
        return instance;
    }

    // We don't want occasional copies
    Context(const Context&) = delete;
    void operator=(const Context&) = delete;

    void Exit() {
        std::unique_ptr<Event> event(new ExitEvent);
        event->type = nncc::context::EventType::Exit;
        Context::Get().GetEventQueue().Push(0, std::move(event));
    }

    bool Init();

    int16_t CreateWindow(const WindowParams& params);

    void DestroyWindow(int16_t idx);

    GLFWwindow* GetGlfwWindow(int16_t window_idx) {
        return windows_[window_idx].get();
    }

    int16_t GetWindowIdx(GLFWwindow* window) {
        return window_indices_.at(window);
    }

    bx::SpScUnboundedQueueT<GlfwMessage>& GetMessageQueue() {
        return glfw_message_queue_;
    }

    EventQueue& GetEventQueue() {
        return event_queue_;
    }

    bx::Thread* GetDefaultThread() {
        return &default_thread_;
    }

    KeyState key_state;
    MouseState mouse_state;

private:

    static bx::DefaultAllocator allocator_;
    bx::SpScUnboundedQueueT<GlfwMessage> glfw_message_queue_;

    EventQueue event_queue_;

    std::vector<GLFWWindowUniquePtr> windows_;
    std::unordered_map<GLFWwindow*, int16_t> window_indices_;

    bx::Thread default_thread_;

    static void GLFWErrorCallback(int error, const char* description);

    static void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t /* modifiers */);

    static void CursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos);

    static void KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t modifiers);
};

}