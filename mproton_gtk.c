// +build linux

#include <JavaScriptCore/JavaScript.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <libappindicator/app-indicator.h>

#include "mproton.h"

static void
xxxxactivate (GtkApplication* app,
          gpointer        user_data)
{

  //__builtin_trap();

  GtkWidget *window;
  GtkWidget *webview;


  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Window");
  gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);


  webview = webkit_web_view_new();
/*
    WebKitUserContentManager *manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
    g_signal_connect(manager, "script-message-received::external",
                     G_CALLBACK(+[](WebKitUserContentManager *, WebKitJavascriptResult *r, gpointer arg) {
                       auto *w = static_cast<gtk_webkit_engine *>(arg);
#if WEBKIT_MAJOR_VERSION >= 2 && WEBKIT_MINOR_VERSION >= 22
                       JSCValue *value = webkit_javascript_result_get_js_value(r);
                       char *s = jsc_value_to_string(value);
#else
                       JSGlobalContextRef ctx = webkit_javascript_result_get_global_context(r);
                       JSValueRef value = webkit_javascript_result_get_value(r);
                       JSStringRef js = JSValueToStringCopy(ctx, value, NULL);
                       size_t n = JSStringGetMaximumUTF8CStringSize(js);
                       char *s = g_new(char, n);
                       JSStringGetUTF8CString(js, s, n);
                       JSStringRelease(js);
#endif
                       w->on_message(s);
                       g_free(s);
                     }),
                     this);
    webkit_user_content_manager_register_script_message_handler(manager, "external");
    init("window.external={invoke:function(s){window.webkit.messageHandlers." "external.postMessage(s);}}");
*/
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(webview));
    gtk_widget_grab_focus(GTK_WIDGET(webview));
    // webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), "https://apple.com");
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), "file:///home/igor/vm/okigan/mproton/cmd/exampleapp/protonappui/dist/index.html");
    

    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, true);
    webkit_settings_set_enable_developer_extras(settings, true);

    gtk_widget_show_all(window);


   /* Indicator */
  GtkWidget *menu_item1 = gtk_menu_item_new_with_label("Test1");
	GtkWidget *menu_item2 = gtk_menu_item_new_with_label("Test2");
	GtkMenu *menu = GTK_MENU(gtk_menu_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item1);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item2);
  gtk_widget_show(menu_item1);
  gtk_widget_show(menu_item2);
  
  AppIndicator * ci = app_indicator_new("test-application",
	                                      "system-shutdown",
	                                      APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
	app_indicator_set_status(ci, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_menu(ci, menu);
}

int
xmain ()
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (xxxxactivate), NULL);
  // char * argv = ["dummy"];
   char *strings[] = {"one","two","three"};
  status = g_application_run (G_APPLICATION (app), 1, strings);


  g_object_unref (app);

  return status;
}

#ifdef STANDALONEPROG
int main(int    argc,
      char **argv){
    return xmain();
}
#endif




int initialize(void) {
    return 0;
}

int set_title (const char* title) {
        return 0;
}

int set_menu_extra_text (const char* text) {
    return 0;
}

int add_menu_extra_item (const char* text) {
    
    return 0;
}

int add_content_path (const char* _Nullable path) {
    return 0;
}

int add_script_message_handler(const char * _Nullable name) {

	return 0;
}

