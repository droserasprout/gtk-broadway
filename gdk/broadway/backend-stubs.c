/* Generated - no-op shims for the whole gdk-x11/gdk-wayland ABI, baked into the
 * broadway-only fork's libgtk-4 so apps and modules built against a full GTK
 * resolve gdk_x11_/gdk_wayland_ at load (they link BIND_NOW). None run on a
 * Broadway display: the _get_type funcs back GDK_IS_X11 / GDK_IS_WAYLAND and
 * returning 0 makes those guards false; the rest is dead code behind them.
 * visibility("default") forces export past the lib's hidden default.
 * Regenerate on re-fork (nm one-liner in devnotes/backend-stubs.md).
 */
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"

__attribute__((visibility("default"))) long gdk_wayland_cairo_context_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_device_get_node_path(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_device_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_device_get_wl_keyboard(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_device_get_wl_pointer(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_device_get_wl_seat(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_device_get_xkb_keymap(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_get_egl_display(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_get_startup_notification_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_get_wl_compositor(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_get_wl_display(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_query_registry(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_set_cursor_theme(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_display_set_startup_notification_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_gl_context_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_monitor_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_monitor_get_wl_output(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_popup_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_seat_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_seat_get_wl_seat(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_surface_force_next_commit(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_surface_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_surface_get_wl_surface(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_toplevel_drop_exported_handle(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_toplevel_export_handle(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_toplevel_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_toplevel_set_application_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_toplevel_set_transient_for_exported(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_toplevel_unexport_handle(void) { return 0; }
__attribute__((visibility("default"))) long gdk_wayland_vulkan_context_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_app_launch_context_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_cairo_context_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_device_get_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_device_manager_lookup(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_device_manager_xi2_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_device_xi2_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_broadcast_startup_message(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_error_trap_pop(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_error_trap_pop_ignored(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_error_trap_push(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_default_group(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_egl_display(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_egl_version(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_glx_version(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_primary_monitor(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_screen(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_startup_notification_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_user_time(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_xcursor(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_xdisplay(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_xrootwindow(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_get_xscreen(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_grab(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_open(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_set_cursor_theme(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_set_program_class(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_set_startup_notification_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_set_surface_scale(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_string_to_compound_text(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_text_property_to_text_list(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_ungrab(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_display_utf8_to_compound_text(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_drag_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_free_compound_text(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_free_text_list(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_get_server_time(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_get_xatom_by_name_for_display(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_get_xatom_name_for_display(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_gl_context_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_lookup_xdisplay(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_monitor_get_output(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_monitor_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_monitor_get_workarea(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_current_desktop(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_monitor_output(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_number_of_desktops(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_screen_number(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_window_manager_name(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_get_xscreen(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_screen_supports_net_wm_hint(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_set_sm_client_id(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_get_desktop(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_get_group(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_get_type(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_get_xid(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_lookup_for_display(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_move_to_current_desktop(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_move_to_desktop(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_frame_sync_enabled(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_group(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_skip_pager_hint(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_skip_taskbar_hint(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_theme_variant(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_urgency_hint(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_user_time(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_surface_set_utf8_property(void) { return 0; }
__attribute__((visibility("default"))) long gdk_x11_vulkan_context_get_type(void) { return 0; }
