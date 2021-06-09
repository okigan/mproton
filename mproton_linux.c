// +build linux

#include <JavaScriptCore/JavaScript.h>
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>
#include <webkit2/webkit2.h>

#include "mproton.h"
#include "mproton_cgo_exports.h"

GtkWidget* main_window = NULL;
WebKitWebView* webview = NULL;

int prtn_event_loop() {
    gtk_widget_grab_focus(GTK_WIDGET(webview));
    gtk_widget_show_all(main_window);
    gtk_main();
}

#ifdef STANDALONEPROG
int main(int argc, char** argv) { return prtn_event_loop(); }
#endif

// https://wiki.gnome.org/Projects/WebKitGtk/ProgrammingGuide/Tutorial

static void destroyWindowCb(GtkWidget* widget, GtkWidget* window) { gtk_main_quit(); }

static gboolean closeWebViewCb(WebKitWebView* webview, GtkWidget* window) {
    gtk_widget_destroy(window);
    return TRUE;
}

static gboolean handle_script_message(WebKitUserContentManager* manager,
                                      WebKitJavascriptResult* message,
                                      gpointer user_data) {
    printf("[c++] in handle_script_message: %s\n", (char*)user_data);

    char* message_str;

    JSCValue* jsc_value = webkit_javascript_result_get_js_value(message);
    const char* str = jsc_value_to_json(jsc_value, 2);
    g_print("Script message received for handler foo: %s\n", str);

    _prtn_call_into_go_with_reply((char*)str);

    return TRUE;
}

int prtn_initialize(void) {
    gtk_init(0, NULL);
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

    WebKitSettings* settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webview));
    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, true);
    webkit_settings_set_enable_developer_extras(settings, true);

    WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
    g_signal_connect(manager, "script-message-received::mycallback1", G_CALLBACK(handle_script_message), NULL);
    webkit_user_content_manager_register_script_message_handler(manager, "mycallback1");

    char * user_script =
        "var _proton_promises = {};  "
        "var _proton_promiseSeq = 0;  "
        "window.proton = {};"
        "function _proton_genPromiseSequenceNumber() {  "
        "    _proton_promiseSeq = _proton_promiseSeq + 1;  "
        "    return _proton_promiseSeq;  "
        "}  "
        "function _proton_resolvePromise(promiseId, data, error) {  "
        "    if (error) {  "
        "        _proton_promises[promiseId].reject(error);  "
        "    } else {  "
        "        _proton_promises[promiseId].resolve(data);  "
        "    }  "
        "    delete _proton_promises[promiseId];  "
        "}  ";


    WebKitUserScript* user_script_obj = webkit_user_script_new(
        user_script,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL,
        NULL);
    webkit_user_content_manager_add_script(manager, user_script_obj);

    // init("window.external={invoke:function(s){window.webkit.messageHandlers."
    // "external.postMessage(s);}}");

    gtk_container_add(GTK_CONTAINER(main_window), GTK_WIDGET(webview));

    g_signal_connect(main_window, "destroy", G_CALLBACK(destroyWindowCb), NULL);
    g_signal_connect(webview, "close", G_CALLBACK(closeWebViewCb), main_window);

    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);
    /* Indicator */
    GtkWidget* menu_item1 = gtk_menu_item_new_with_label("Test1");
    GtkWidget* menu_item2 = gtk_menu_item_new_with_label("Test2");
    GtkMenu* menu = GTK_MENU(gtk_menu_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item1);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item2);
    gtk_widget_show(menu_item1);
    gtk_widget_show(menu_item2);

    AppIndicator* ci =
        app_indicator_new("test-application", "system-shutdown", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(ci, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_menu(ci, menu);
    return 0;
}

int prtn_set_title(const char* title) {
    gtk_window_set_title(GTK_WINDOW(main_window), title);
    return 0;
}

int prtn_set_menu_extra_text(const char* text) { return 0; }

int prtn_add_menu_extra_item(const char* text, int tag) { return 0; }

int prtn_set_content(const char* path) {
    webkit_web_view_load_html(webview, path, NULL);

    return 0;
}

int prtn_set_content_path(const char* path) {
    webkit_web_view_load_uri(webview, path);

    return 0;
}

int prtn_add_script_message_handler(const char* name) {
    char script[2048] = "\0";
    snprintf(script,
             sizeof(script) / sizeof(script[0]),
                      "function _proton_%s_invoke(s) {  \n"
                      "    var args = arguments;  \n"
                      "    var promise = new Promise(function(resolve, reject) {  \n"
                      "        var promiseId = _proton_genPromiseSequenceNumber();  \n"
                      "        _proton_promises[promiseId] = { resolve, reject };  \n"
                      "        window.webkit.messageHandlers.%s.postMessage({ promiseId: "
                      "promiseId, name: \"%s\", param: Array.prototype.slice.call(args) });  \n"
                      "    });  \n"
                      "    return promise;  \n"
                      "}  \n"
                      "  \n"
                      "window.proton['%s'] = {  \n"
                      "    invoke: _proton_%s_invoke  \n"
                      "}  ",
                      name,
                      name,
                      name,
                      name,
                      name);

    WebKitUserScript* user_script =
        webkit_user_script_new(script,
                               WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
                               WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
                               NULL,
                               NULL);

    WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview));
    webkit_user_content_manager_add_script(manager, user_script);

    return 0;
}

int prtn_execute_script(const char* script) {
    printf("in prtn_execute_script");

    webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(webview), script, NULL, NULL, NULL);

    return 0;
}

int prtn_resolve(const char* promiseId, const char* result, const char* error) {
    printf("in prtn_resolve: %s, %s, %s\n", promiseId, result, error);

    char buf[4 * 1024] = "\n";
    if (error != NULL) {
        sprintf(buf, "_proton_resolvePromise(%s,null, '%s');", promiseId, error);
    } else {
        sprintf(buf, "_proton_resolvePromise(%s,'%s', null);", promiseId, result);
    }

    prtn_execute_script(buf);

    return 0;
}