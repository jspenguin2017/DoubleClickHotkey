#include "platform/windows/main_window.hpp"

#include "platform/windows/clock.hpp"
#include "platform/windows/input_threads.hpp"
#include "platform/windows/resource.h"
#include "platform/windows/single_instance.hpp"

#include <algorithm>
#include <commctrl.h>
#include <utility>

namespace double_click_hotkey::windows
{
namespace
{
constexpr UINT_PTR CountdownTimer = 1;
constexpr int DelayControl = 101;
constexpr int SendControl = 102;
constexpr UINT QuitCommand = 0x1000;
constexpr DWORD WindowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
int Scale(const int value, const UINT dpi)
{
    return MulDiv(value, static_cast<int>(dpi), 96);
}
std::wstring ReadText(HWND window)
{
    const auto length = GetWindowTextLengthW(window);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    const auto copied = GetWindowTextW(window, text.data(), static_cast<int>(text.size()));
    text.resize(static_cast<std::size_t>(copied));
    return text;
}
} // namespace

MainWindow::MainWindow(EventHandler handler) : handler_(std::move(handler))
{
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_STANDARD_CLASSES};
    Require(InitCommonControlsEx(&common) != FALSE, "Initialize native controls");
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = WindowProc;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    cls.lpszClassName = MainWindowClass;
    Require(RegisterClassExW(&cls) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS, "Register main window");
    // No WS_VISIBLE: neither the main window nor a console flashes during startup.
    window_.Reset(CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"Double Click Hotkey", WindowStyle,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 740, 480, nullptr, nullptr, cls.hInstance, this));
    Require(window_.Get() != nullptr, "Create main window");
    const auto system_menu = GetSystemMenu(window_.Get(), FALSE);
    Require(AppendMenuW(system_menu, MF_SEPARATOR, 0, nullptr) != FALSE, "Extend system menu");
    Require(AppendMenuW(system_menu, MF_STRING, QuitCommand, L"Quit Double Click Hotkey") != FALSE, "Add Quit action");
    const auto add = [this, &cls](DWORD ex_style, const wchar_t* class_name, const wchar_t* text, DWORD style, int id) {
        const auto control =
            CreateWindowExW(ex_style, class_name, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, window_.Get(),
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), cls.hInstance, nullptr);
        Require(control != nullptr, "Create window control");
        return control;
    };
    log_ = add(WS_EX_CLIENTEDGE, L"EDIT", L"",
               WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
                   ES_NOHIDESEL,
               100);
    SendMessageW(log_, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);
    delay_label_ = add(0, L"STATIC", L"&Delay (seconds):", SS_LEFT, 103);
    delay_ = add(WS_EX_CLIENTEDGE, L"EDIT", L"5", WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER, DelayControl);
    SendMessageW(delay_, EM_SETLIMITTEXT, 10, 0);
    send_ = add(0, L"BUTTON", L"Send F13", WS_TABSTOP | BS_PUSHBUTTON, SendControl);
    const auto dpi = GetDpiForWindow(window_.Get());
    RECT rect{0, 0, Scale(720, dpi), Scale(420, dpi)};
    Require(AdjustWindowRectExForDpi(&rect, WindowStyle, FALSE, WS_EX_CONTROLPARENT, dpi) != FALSE, "Size main window");
    SetWindowPos(window_.Get(), nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    UpdateDpi();
    KeepOnWorkArea();
}

MainWindow::~MainWindow()
{
    ClearHandler();
    tray_ = nullptr;
    input_threads_ = nullptr;
    CancelTimer();
    window_.Reset();
}
HWND MainWindow::Get() const noexcept
{
    return window_.Get();
}
HICON MainWindow::SmallIcon() const noexcept
{
    return small_icon_.Get();
}
void MainWindow::SetTray(TrayIcon* tray) noexcept
{
    tray_ = tray;
}
void MainWindow::SetInputThreads(InputThreads* input_threads) noexcept
{
    input_threads_ = input_threads;
}
void MainWindow::ClearHandler() noexcept
{
    handler_ = {};
}
std::exception_ptr MainWindow::Failure() const noexcept
{
    return failure_;
}

void MainWindow::Emit(const EventKind kind, std::string text)
{
    if (handler_)
        handler_({kind, std::move(text)});
}

void MainWindow::CancelTimer() noexcept
{
    if (window_.Get())
        KillTimer(window_.Get(), CountdownTimer);
    timer_deadline_.reset();
}

PlatformResult MainWindow::ScheduleTick(const std::optional<ElapsedTime> deadline)
{
    if (deadline == timer_deadline_)
        return {};
    CancelTimer();
    if (deadline)
    {
        const auto delay =
            std::clamp(*deadline - InterruptTime(), ElapsedTime(USER_TIMER_MINIMUM), ElapsedTime(USER_TIMER_MAXIMUM));
        if (!SetTimer(window_.Get(), CountdownTimer, static_cast<UINT>(delay.count()), nullptr))
            return {false, NativeError("Schedule F13 countdown")};
    }
    timer_deadline_ = deadline;
    return {};
}

void MainWindow::Layout()
{
    if (!send_)
        return;
    const auto dpi = GetDpiForWindow(window_.Get());
    RECT client{};
    GetClientRect(window_.Get(), &client);
    const int margin = Scale(12, dpi);
    const int height = Scale(28, dpi);
    const int row = client.bottom - margin - height;
    MoveWindow(log_, margin, margin, std::max(1L, client.right - 2 * margin), std::max(1, row - 2 * margin), TRUE);
    MoveWindow(delay_label_, margin + Scale(2, dpi), row + Scale(5, dpi), Scale(112, dpi), height, TRUE);
    MoveWindow(delay_, margin + Scale(116, dpi), row, Scale(72, dpi), height, TRUE);
    MoveWindow(send_, client.right - margin - Scale(180, dpi), row, Scale(180, dpi), height, TRUE);
}

void MainWindow::UpdateDpi()
{
    const auto dpi = GetDpiForWindow(window_.Get());
    LOGFONTW font{};
    font.lfHeight = -Scale(13, dpi);
    font.lfWeight = FW_NORMAL;
    lstrcpyW(font.lfFaceName, L"Segoe UI");
    UniqueFont ui(CreateFontIndirectW(&font));
    lstrcpyW(font.lfFaceName, L"Consolas");
    UniqueFont log(CreateFontIndirectW(&font));
    Require(ui.Get() && log.Get(), "Create window fonts");
    for (auto control : {delay_label_, delay_, send_})
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(ui.Get()), TRUE);
    SendMessageW(log_, WM_SETFONT, reinterpret_cast<WPARAM>(log.Get()), TRUE);
    ui_font_.Reset(ui.Release());
    log_font_.Reset(log.Release());
    const auto instance = GetModuleHandleW(nullptr);
    UniqueIcon small(static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_DOUBLE_CLICK_HOTKEY), IMAGE_ICON,
                                                   GetSystemMetricsForDpi(SM_CXSMICON, dpi),
                                                   GetSystemMetricsForDpi(SM_CYSMICON, dpi), 0)));
    UniqueIcon large(static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_DOUBLE_CLICK_HOTKEY), IMAGE_ICON,
                                                   GetSystemMetricsForDpi(SM_CXICON, dpi),
                                                   GetSystemMetricsForDpi(SM_CYICON, dpi), 0)));
    Require(small.Get() && large.Get(), "Load embedded icons");
    SendMessageW(window_.Get(), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(small.Get()));
    SendMessageW(window_.Get(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(large.Get()));
    if (tray_)
        tray_->UpdateIcon(small.Get());
    small_icon_.Reset(small.Release());
    large_icon_.Reset(large.Release());
    Layout();
}

void MainWindow::KeepOnWorkArea()
{
    if (IsIconic(window_.Get()))
        return;
    MONITORINFO monitor{sizeof(monitor), {}, {}, 0};
    RECT rect{};
    if (GetMonitorInfoW(MonitorFromWindow(window_.Get(), MONITOR_DEFAULTTONEAREST), &monitor) &&
        GetWindowRect(window_.Get(), &rect))
    {
        const auto x = std::clamp(rect.left, monitor.rcWork.left,
                                  std::max(monitor.rcWork.left, monitor.rcWork.right - (rect.right - rect.left)));
        const auto y = std::clamp(rect.top, monitor.rcWork.top,
                                  std::max(monitor.rcWork.top, monitor.rcWork.bottom - (rect.bottom - rect.top)));
        SetWindowPos(window_.Get(), nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void MainWindow::RestoreFocus()
{
    SetFocus(IsChild(window_.Get(), last_focus_) && IsWindowEnabled(last_focus_) ? last_focus_ : log_);
}

void MainWindow::SetVisible(const bool visible)
{
    if (visible)
    {
        ShowWindow(window_.Get(), IsIconic(window_.Get()) ? SW_RESTORE : SW_SHOW);
        KeepOnWorkArea();
        SetForegroundWindow(window_.Get());
        RestoreFocus();
    }
    else
    {
        const auto focus = GetFocus();
        if (IsChild(window_.Get(), focus))
            last_focus_ = focus;
        ShowWindow(window_.Get(), SW_HIDE);
    }
}

void MainWindow::UpdateLog(const ViewState& state)
{
    if (state.log_revision == log_revision_)
        return;
    DWORD selection_start = 0, selection_end = 0;
    SendMessageW(log_, EM_GETSEL, reinterpret_cast<WPARAM>(&selection_start), reinterpret_cast<LPARAM>(&selection_end));
    SCROLLINFO scroll{};
    scroll.cbSize = sizeof(scroll);
    scroll.fMask = SIF_ALL;
    GetScrollInfo(log_, SB_VERT, &scroll);
    const bool follow =
        selection_start == selection_end && scroll.nPos >= scroll.nMax - static_cast<int>(scroll.nPage) + 1;
    const auto first_line = static_cast<int>(SendMessageW(log_, EM_GETFIRSTVISIBLELINE, 0, 0));
    const int horizontal = GetScrollPos(log_, SB_HORZ);
    const auto removed = state.removed_lines - removed_lines_;
    std::size_t removed_characters = 0;
    for (std::uint64_t line = 0; line < removed && removed_characters < log_text_.size(); ++line)
    {
        const auto end = log_text_.find(L'\n', removed_characters);
        removed_characters = end == std::wstring::npos ? log_text_.size() : end + 1;
    }
    std::wstring text;
    const auto wide = ToWide(state.log_text);
    for (const auto character : wide)
    {
        if (character == L'\n')
            text += L'\r';
        text += character;
    }
    // Avoid WM_SETREDRAW on a hidden control: reenabling it changes WS_VISIBLE on Windows.
    const bool redraw = IsWindowVisible(log_) != FALSE;
    if (redraw)
        SendMessageW(log_, WM_SETREDRAW, FALSE, 0);
    Require(SetWindowTextW(log_, text.c_str()) != FALSE, "Update log text");
    if (follow)
    {
        SendMessageW(log_, EM_SETSEL, text.size(), text.size());
        SendMessageW(log_, EM_SCROLLCARET, 0, 0);
    }
    else
    {
        const auto adjust = [removed_characters, &text](DWORD position) {
            return std::min(text.size(), position > removed_characters ? position - removed_characters : 0);
        };
        SendMessageW(log_, EM_SETSEL, adjust(selection_start), adjust(selection_end));
        const int target_line = first_line - static_cast<int>(std::min<std::uint64_t>(removed, first_line));
        const auto current_line = static_cast<int>(SendMessageW(log_, EM_GETFIRSTVISIBLELINE, 0, 0));
        SendMessageW(log_, EM_LINESCROLL, 0, target_line - current_line);
        SendMessageW(log_, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, horizontal), 0);
    }
    if (redraw)
    {
        SendMessageW(log_, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(log_, nullptr, TRUE);
    }
    log_text_ = std::move(text);
    removed_lines_ = state.removed_lines;
    log_revision_ = state.log_revision;
}

void MainWindow::Present(const ViewState& state)
{
    presenting_ = true;
    try
    {
        UpdateLog(state);
        const auto delay = ToWide(state.delay_text);
        if (ReadText(delay_) != delay)
            SetWindowTextW(delay_, delay.c_str());
        const auto caption = ToWide(state.send_caption);
        if (ReadText(send_) != caption)
            SetWindowTextW(send_, caption.c_str());
        EnableWindow(delay_, state.delay_enabled);
        EnableWindow(send_, state.send_enabled);
        presenting_ = false;
    }
    catch (...)
    {
        presenting_ = false;
        throw;
    }
}

bool MainWindow::PreTranslate(MSG& message)
{
    if (message.message == WM_KEYDOWN)
    {
        if ((message.hwnd == log_ || message.hwnd == delay_) && message.wParam == 'A' && GetKeyState(VK_CONTROL) < 0)
        {
            SendMessageW(message.hwnd, EM_SETSEL, 0, -1);
            return true;
        }
        if (message.hwnd == send_ && message.wParam == VK_RETURN && IsWindowEnabled(send_))
        {
            SendMessageW(send_, BM_CLICK, 0, 0);
            return true;
        }
    }
    return IsWindowVisible(window_.Get()) && IsDialogMessageW(window_.Get(), &message);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) noexcept
{
    auto* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        self = static_cast<MainWindow*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self)
    {
        try
        {
            return self->Message(window, message, wparam, lparam);
        }
        catch (...)
        {
            self->failure_ = std::current_exception();
            self->CancelTimer();
            if (self->input_threads_)
                self->input_threads_->RequestStop();
            PostQuitMessage(1);
            return 0;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT MainWindow::Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (tray_ && message == tray_->TaskbarMessage())
    {
        if (!tray_->Add())
        {
            Emit(EventKind::diagnostic, "The tray icon could not be restored. Use Quit Double Click Hotkey in the "
                                        "window's system menu to exit.");
            Emit(EventKind::show);
        }
        return 0;
    }
    switch (message)
    {
    case WM_COMMAND:
        if (!presenting_)
        {
            if (LOWORD(wparam) == DelayControl && HIWORD(wparam) == EN_CHANGE && delay_)
                Emit(EventKind::delay_changed, ToUtf8(ReadText(delay_)));
            else if (LOWORD(wparam) == SendControl && HIWORD(wparam) == BN_CLICKED)
                Emit(EventKind::send_requested);
        }
        return 0;
    case InputThreads::DoubleClickFailureMessage:
        if (input_threads_)
            Emit(EventKind::diagnostic, InputThreads::DoubleClickFailureText(wparam, lparam));
        return 0;
    case InputThreads::WorkerFailureMessage:
        if (input_threads_)
            input_threads_->CheckFailure();
        return 0;
    case WM_TIMER:
        if (wparam == CountdownTimer && timer_deadline_)
        {
            CancelTimer();
            Emit(EventKind::tick);
        }
        return 0;
    case WM_CLOSE:
        Emit(tray_ && !tray_->Available() ? EventKind::show : EventKind::hide);
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (wparam)
            Emit(EventKind::quit);
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xFFF0) == QuitCommand)
        {
            Emit(EventKind::quit);
            return 0;
        }
        break;
    case WM_SIZE:
        Layout();
        return 0;
    case WM_GETMINMAXINFO: {
        const auto dpi = GetDpiForWindow(window);
        auto* limits = reinterpret_cast<MINMAXINFO*>(lparam);
        limits->ptMinTrackSize = {Scale(460, dpi), Scale(260, dpi)};
        return 0;
    }
    case WM_DPICHANGED:
        if (send_)
        {
            const auto* rect = reinterpret_cast<RECT*>(lparam);
            SetWindowPos(window, nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            UpdateDpi();
            KeepOnWorkArea();
        }
        return 0;
    case WM_SETTINGCHANGE:
    case WM_DISPLAYCHANGE:
        if (window_.Get())
            KeepOnWorkArea();
        break;
    case WM_ACTIVATE:
        if (LOWORD(wparam) == WA_INACTIVE && IsChild(window, GetFocus()))
            last_focus_ = GetFocus();
        break;
    case WM_SETFOCUS:
        if (log_)
            RestoreFocus();
        return 0;
    case tray_message:
        if (tray_)
        {
            const auto event = tray_->Handle(lparam);
            if (event)
                Emit(*event);
        }
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}
} // namespace double_click_hotkey::windows
