// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_tray.h"

#include <string>

#include "base/threading/thread_task_runner_handle.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/api/ui_event.h"
#include "shell/browser/browser.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/guid_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/node_includes.h"
#include "ui/gfx/image/image.h"

namespace gin {

template <>
struct Converter<electron::TrayIcon::IconType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::TrayIcon::IconType* out) {
    using IconType = electron::TrayIcon::IconType;
    std::string mode;
    if (ConvertFromV8(isolate, val, &mode)) {
      if (mode == "none") {
        *out = IconType::None;
        return true;
      } else if (mode == "info") {
        *out = IconType::Info;
        return true;
      } else if (mode == "warning") {
        *out = IconType::Warning;
        return true;
      } else if (mode == "error") {
        *out = IconType::Error;
        return true;
      } else if (mode == "custom") {
        *out = IconType::Custom;
        return true;
      }
    }
    return false;
  }
};

}  // namespace gin

namespace electron {

namespace api {

gin::WrapperInfo Tray::kWrapperInfo = {gin::kEmbedderNativeGin};

Tray::Tray(gin::Handle<NativeImage> image,
           base::Optional<UUID> guid,
           gin::Arguments* args)
    : tray_icon_(TrayIcon::Create(guid)) {
  SetImage(image);
  tray_icon_->AddObserver(this);
}

Tray::~Tray() = default;

// static
gin::Handle<Tray> Tray::New(gin_helper::ErrorThrower thrower,
                            gin::Handle<NativeImage> image,
                            base::Optional<UUID> guid,
                            gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create Tray before app is ready");
    return gin::Handle<Tray>();
  }

#if defined(OS_WIN)
  if (!guid.has_value() && args->Length() > 1) {
    thrower.ThrowError("Invalid GUID format");
    return gin::Handle<Tray>();
  }
#endif

  return gin::CreateHandle(thrower.isolate(), new Tray(image, guid, args));
}

void Tray::OnClicked(const gfx::Rect& bounds,
                     const gfx::Point& location,
                     int modifiers) {
  EmitCustomEvent("click", CreateEventFromFlags(modifiers), bounds, location);
}

void Tray::OnDoubleClicked(const gfx::Rect& bounds, int modifiers) {
  EmitCustomEvent("double-click", CreateEventFromFlags(modifiers), bounds);
}

void Tray::OnRightClicked(const gfx::Rect& bounds, int modifiers) {
  EmitCustomEvent("right-click", CreateEventFromFlags(modifiers), bounds);
}

void Tray::OnBalloonShow() {
  Emit("balloon-show");
}

void Tray::OnBalloonClicked() {
  Emit("balloon-click");
}

void Tray::OnBalloonClosed() {
  Emit("balloon-closed");
}

void Tray::OnDrop() {
  Emit("drop");
}

void Tray::OnDropFiles(const std::vector<std::string>& files) {
  Emit("drop-files", files);
}

void Tray::OnDropText(const std::string& text) {
  Emit("drop-text", text);
}

void Tray::OnMouseEntered(const gfx::Point& location, int modifiers) {
  EmitCustomEvent("mouse-enter", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseExited(const gfx::Point& location, int modifiers) {
  EmitCustomEvent("mouse-leave", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseMoved(const gfx::Point& location, int modifiers) {
  EmitCustomEvent("mouse-move", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseUp(const gfx::Point& location, int modifiers) {
  EmitCustomEvent("mouse-up", CreateEventFromFlags(modifiers), location);
}

void Tray::OnMouseDown(const gfx::Point& location, int modifiers) {
  EmitCustomEvent("mouse-down", CreateEventFromFlags(modifiers), location);
}

void Tray::OnDragEntered() {
  Emit("drag-enter");
}

void Tray::OnDragExited() {
  Emit("drag-leave");
}

void Tray::OnDragEnded() {
  Emit("drag-end");
}

void Tray::Destroy() {
  menu_.Reset();
  tray_icon_.reset();
}

bool Tray::IsDestroyed() {
  return !tray_icon_;
}

void Tray::SetImage(gin::Handle<NativeImage> image) {
  if (!CheckDestroyed())
    return;
#if defined(OS_WIN)
  tray_icon_->SetImage(image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetImage(image->image());
#endif
}

void Tray::SetPressedImage(gin::Handle<NativeImage> image) {
  if (!CheckDestroyed())
    return;
#if defined(OS_WIN)
  tray_icon_->SetPressedImage(image->GetHICON(GetSystemMetrics(SM_CXSMICON)));
#else
  tray_icon_->SetPressedImage(image->image());
#endif
}

void Tray::SetToolTip(const std::string& tool_tip) {
  if (!CheckDestroyed())
    return;
  tray_icon_->SetToolTip(tool_tip);
}

void Tray::SetTitle(const std::string& title) {
  if (!CheckDestroyed())
    return;
#if defined(OS_MACOSX)
  tray_icon_->SetTitle(title);
#endif
}

std::string Tray::GetTitle() {
  if (!CheckDestroyed())
    return std::string();
#if defined(OS_MACOSX)
  return tray_icon_->GetTitle();
#else
  return "";
#endif
}

void Tray::SetIgnoreDoubleClickEvents(bool ignore) {
  if (!CheckDestroyed())
    return;
#if defined(OS_MACOSX)
  tray_icon_->SetIgnoreDoubleClickEvents(ignore);
#endif
}

bool Tray::GetIgnoreDoubleClickEvents() {
  if (!CheckDestroyed())
    return false;
#if defined(OS_MACOSX)
  return tray_icon_->GetIgnoreDoubleClickEvents();
#else
  return false;
#endif
}

void Tray::DisplayBalloon(gin_helper::ErrorThrower thrower,
                          const gin_helper::Dictionary& options) {
  if (!CheckDestroyed())
    return;
  TrayIcon::BalloonOptions balloon_options;

  if (!options.Get("title", &balloon_options.title) ||
      !options.Get("content", &balloon_options.content)) {
    thrower.ThrowError("'title' and 'content' must be defined");
    return;
  }

  gin::Handle<NativeImage> icon;
  options.Get("icon", &icon);
  options.Get("iconType", &balloon_options.icon_type);
  options.Get("largeIcon", &balloon_options.large_icon);
  options.Get("noSound", &balloon_options.no_sound);
  options.Get("respectQuietTime", &balloon_options.respect_quiet_time);

  if (!icon.IsEmpty()) {
#if defined(OS_WIN)
    balloon_options.icon = icon->GetHICON(
        GetSystemMetrics(balloon_options.large_icon ? SM_CXICON : SM_CXSMICON));
#else
    balloon_options.icon = icon->image();
#endif
  }

  tray_icon_->DisplayBalloon(balloon_options);
}

void Tray::RemoveBalloon() {
  if (!CheckDestroyed())
    return;
  tray_icon_->RemoveBalloon();
}

void Tray::Focus() {
  if (!CheckDestroyed())
    return;
  tray_icon_->Focus();
}

void Tray::PopUpContextMenu(gin::Arguments* args) {
  if (!CheckDestroyed())
    return;
  gin::Handle<Menu> menu;
  args->GetNext(&menu);
  gfx::Point pos;
  args->GetNext(&pos);
  tray_icon_->PopUpContextMenu(pos, menu.IsEmpty() ? nullptr : menu->model());
}

void Tray::CloseContextMenu() {
  if (!CheckDestroyed())
    return;
  tray_icon_->CloseContextMenu();
}

void Tray::SetContextMenu(gin_helper::ErrorThrower thrower,
                          v8::Local<v8::Value> arg) {
  if (!CheckDestroyed())
    return;
  gin::Handle<Menu> menu;
  if (arg->IsNull()) {
    menu_.Reset();
    tray_icon_->SetContextMenu(nullptr);
  } else if (gin::ConvertFromV8(thrower.isolate(), arg, &menu)) {
    menu_.Reset(thrower.isolate(), menu.ToV8());
    tray_icon_->SetContextMenu(menu->model());
  } else {
    thrower.ThrowTypeError("Must pass Menu or null");
  }
}

gfx::Rect Tray::GetBounds() {
  if (!CheckDestroyed())
    return gfx::Rect();
  return tray_icon_->GetBounds();
}

bool Tray::CheckDestroyed() {
  if (!tray_icon_) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError("Tray is destroyed");
    return false;
  }
  return true;
}

// static
gin::ObjectTemplateBuilder Tray::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<Tray>::GetObjectTemplateBuilder(isolate)
      .SetMethod("destroy", &Tray::Destroy)
      .SetMethod("isDestroyed", &Tray::IsDestroyed)
      .SetMethod("setImage", &Tray::SetImage)
      .SetMethod("setPressedImage", &Tray::SetPressedImage)
      .SetMethod("setToolTip", &Tray::SetToolTip)
      .SetMethod("setTitle", &Tray::SetTitle)
      .SetMethod("getTitle", &Tray::GetTitle)
      .SetMethod("setIgnoreDoubleClickEvents",
                 &Tray::SetIgnoreDoubleClickEvents)
      .SetMethod("getIgnoreDoubleClickEvents",
                 &Tray::GetIgnoreDoubleClickEvents)
      .SetMethod("displayBalloon", &Tray::DisplayBalloon)
      .SetMethod("removeBalloon", &Tray::RemoveBalloon)
      .SetMethod("focus", &Tray::Focus)
      .SetMethod("popUpContextMenu", &Tray::PopUpContextMenu)
      .SetMethod("closeContextMenu", &Tray::CloseContextMenu)
      .SetMethod("setContextMenu", &Tray::SetContextMenu)
      .SetMethod("getBounds", &Tray::GetBounds);
}

const char* Tray::GetTypeName() {
  return "Tray";
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::Tray;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("createTray", &Tray::New);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_tray, Initialize)
